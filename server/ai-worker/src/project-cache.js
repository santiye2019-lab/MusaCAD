import { planProjectRetention, compareManifestRevision, MAX_OPEN_PROJECTS }
  from "./project-retention.js";

// The project cache is DISABLED until private R2 and Durable Object bindings
// have been provisioned and MUSACAD_AUTOUPLOAD_ENABLED is set to "true".
const CHUNK_BYTES=1024*1024;
const MAX_FILE_BYTES=512*1024*1024;
const MAX_PROJECTS=12;
const ID=/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
const SHA=/^[0-9a-f]{64}$/i;
const json=(data,status=200)=>new Response(JSON.stringify(data),{
  status,headers:{"content-type":"application/json; charset=utf-8","cache-control":"no-store",
    "x-content-type-options":"nosniff"}
});
const error=(status,message)=>json({status:"error",message},status);
const goodId=v=>typeof v==="string"&&ID.test(v);
const validName=v=>typeof v==="string"&&v.length>0&&v.length<=180&&
  !/[\x00-\x1f\x7f]/.test(v)&&/\.(dwg|dxf)$/i.test(v);
async function shaBytes(buf){
  return [...new Uint8Array(await crypto.subtle.digest("SHA-256",buf))]
    .map(x=>x.toString(16).padStart(2,"0")).join("");
}
async function readBounded(request,max){
  const size=Number(request.headers.get("content-length"));
  if(Number.isFinite(size)&&size>max)throw new RangeError("body too large");
  if(!request.body)return new Uint8Array();
  const reader=request.body.getReader();
  const chunks=[];let length=0;
  try {
    while(true){
      const {done,value}=await reader.read();if(done)break;
      length+=value.byteLength;
      if(length>max)throw new RangeError("body too large");
      chunks.push(value);
    }
  }finally{reader.releaseLock();}
  const output=new Uint8Array(length);let offset=0;
  for(const chunk of chunks){output.set(chunk,offset);offset+=chunk.byteLength;}
  return output;
}
async function readBody(request,max=4096){
  const text=new TextDecoder().decode(await readBounded(request,max));
  return JSON.parse(text);
}

/**
 * Called by public Worker only AFTER signed MAI1/MAI2 authentication.
 * The forwarded device header is replaced, never accepted from the client.
 */
export async function handleProjectCache(request,env,verifySession){
  if(env.MUSACAD_AUTOUPLOAD_ENABLED!=="true"||
     !env.MUSACAD_PROJECT_BUCKET||!env.MUSACAD_PROJECT_COORDINATOR||
     !env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM)
    return error(503,"Proje deposu henüz yapılandırılmadı");
  const session=await verifySession(request.headers.get("authorization"),
    env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM);
  if(!session.ok)return error(401,"AI oturumu doğrulanamadı");
  const objectId=env.MUSACAD_PROJECT_COORDINATOR.idFromName(session.deviceId);
  const coordinator=env.MUSACAD_PROJECT_COORDINATOR.get(objectId);
  const headers=new Headers(request.headers);
  headers.delete("authorization");
  headers.set("x-musacad-verified-device-id",session.deviceId);
  return coordinator.fetch(new Request(request,{headers}));
}

/**
 * One coordinator per signed device. Its async request queue serializes all
 * open-tab revisions, verified chunk writes, finalizations and R2 deletions.
 * The durable catalog survives Worker restarts; R2 remains private.
 */
export class MusaCadProjectCoordinator {
  constructor(state,env){
    this.state=state;this.env=env;this.queue=Promise.resolve();
  }
  fetch(request){
    const next=this.queue.then(()=>this.route(request));
    this.queue=next.catch(()=>{});
    return next;
  }
  async route(request){
    if(!this.env.MUSACAD_PROJECT_BUCKET)return error(503,"private bucket missing");
    const deviceId=request.headers.get("x-musacad-verified-device-id");
    if(!deviceId)return error(401,"not authenticated");
    const prefix="devices/"+await shaBytes(new TextEncoder().encode(deviceId))+"/";
    const url=new URL(request.url);const path=url.pathname;
    const catalog=await this.state.storage.get("catalog")||{
      revision:0,openProjectIds:[],projects:{},pendingDeletes:[]
    };
    // Physical deletion is retried after durable catalog updates. Old UUID+uploadId
    // paths never overlap a newly opened project version.
    await this.drainDeletes(catalog,prefix);
    try{
      if(path==="/v1/projects/sync-open"&&request.method==="POST"){
        const body=await readBody(request);
        if(!Array.isArray(body.openProjectIds)||body.openProjectIds.length>MAX_OPEN_PROJECTS||
           !body.openProjectIds.every(goodId)||new Set(body.openProjectIds.map(x=>x.toLowerCase())).size!==body.openProjectIds.length)
          return error(400,"geçersiz açık proje listesi");
        const revision=body.revision;
        const step=compareManifestRevision(catalog.revision,revision);
        if(step==="stale")return error(409,"eski sekme revizyonu");
        const ids=body.openProjectIds.map(x=>x.toLowerCase());
        if(step==="idempotent"&&JSON.stringify(ids)!==JSON.stringify(catalog.openProjectIds))
          return error(409,"revizyon içeriği uyuşmuyor");
        if(step==="advance"){
          catalog.revision=revision;
          catalog.openProjectIds=ids;
          await this.cleanup(catalog);
          await this.state.storage.put("catalog",catalog);
          await this.drainDeletes(catalog,prefix);
        }
        return json({status:"ok",revision:catalog.revision,
          openProjectIds:catalog.openProjectIds,retainedProjects:Object.values(catalog.projects)
            .filter(x=>x.status==="complete").map(x=>x.id)});
      }
      if(path==="/v1/projects/status"&&request.method==="GET")
        return json({status:"ok",revision:catalog.revision,
          openProjectIds:catalog.openProjectIds,
          projects:Object.values(catalog.projects).map(x=>({
            id:x.id,status:x.status,receivedParts:Object.keys(x.parts).map(Number),
            partCount:x.partCount,sizeBytes:x.sizeBytes,sha256:x.sha256
          }))});
      if(path==="/v1/projects/init"&&request.method==="POST"){
        const data=await readBody(request);
        if(!goodId(data.projectId)||!validName(data.fileName)||!SHA.test(String(data.sha256||""))||
          !Number.isSafeInteger(data.sizeBytes)||data.sizeBytes<1||data.sizeBytes>MAX_FILE_BYTES)
          return error(400,"geçersiz proje meta verisi");
        const id=data.projectId.toLowerCase();
        if(!catalog.openProjectIds.includes(id))return error(409,"proje açık sekmelerde bulunmuyor");
        if(Object.keys(catalog.projects).length>=MAX_PROJECTS&&!catalog.projects[id])
          return error(429,"cihaz proje kotası dolu");
        let existing=catalog.projects[id];
        if(existing){
          if(existing.sha256!==data.sha256.toLowerCase()||existing.sizeBytes!==data.sizeBytes)
            return error(409,"değişen proje için yeni yükleme kimliği gerekir");
        }else{
          existing={id,fileName:data.fileName,sizeBytes:data.sizeBytes,
            sha256:data.sha256.toLowerCase(),uploadId:crypto.randomUUID(),
            partCount:Math.ceil(data.sizeBytes/CHUNK_BYTES),parts:{},
            status:"uploading",completedAtMs:0};
          catalog.projects[id]=existing;
          await this.state.storage.put("catalog",catalog);
        }
        return json({status:"ok",projectId:id,chunkBytes:CHUNK_BYTES,
          partCount:existing.partCount,receivedParts:Object.keys(existing.parts).map(Number),
          complete:existing.status==="complete"});
      }
      const chunk=/^\/v1\/projects\/([0-9a-f-]{36})\/chunks\/(\d{1,4})$/i.exec(path);
      if(chunk&&request.method==="PUT"){
        const id=chunk[1].toLowerCase();const part=Number(chunk[2]);
        const p=catalog.projects[id];
        if(!p||!catalog.openProjectIds.includes(id))return error(404,"aktif proje bulunamadı");
        if(p.status==="complete")return error(409,"proje zaten tamamlandı");
        if(!Number.isSafeInteger(part)||part<0||part>=p.partCount)
          return error(400,"geçersiz parça numarası");
        const expected=part===p.partCount-1?p.sizeBytes-part*CHUNK_BYTES:CHUNK_BYTES;
        const sha=String(request.headers.get("x-chunk-sha256")||"").toLowerCase();
        if(!SHA.test(sha))return error(400,"parça SHA-256 gereklidir");
        const payload=await readBounded(request,CHUNK_BYTES);
        if(payload.length!==expected)return error(400,"parça boyutu uyuşmuyor");
        const actual=await shaBytes(payload);
        if(sha!==actual)return error(422,"parça SHA-256 uyuşmuyor");
        if(p.parts[part]===sha)return json({status:"ok",part,received:true,reused:true});
        const key=prefix+"projects/"+id+"/"+p.uploadId+"/parts/"+part;
        await this.env.MUSACAD_PROJECT_BUCKET.put(key,payload);
        p.parts[part]=sha;
        await this.state.storage.put("catalog",catalog);
        return json({status:"ok",part,received:true});
      }
      const complete=/^\/v1\/projects\/([0-9a-f-]{36})\/complete$/i.exec(path);
      if(complete&&request.method==="POST"){
        const id=complete[1].toLowerCase();const p=catalog.projects[id];
        if(!p||!catalog.openProjectIds.includes(id))return error(404,"aktif proje bulunamadı");
        if(p.status==="complete")return json({status:"ok",projectId:id,complete:true});
        if(Object.keys(p.parts).length!==p.partCount)
          return error(409,"doğrulanmamış veri parçaları var");
        for(let i=0;i<p.partCount;i++)if(!SHA.test(p.parts[i]||""))
          return error(409,"eksik parça SHA-256");
        p.status="complete";p.completedAtMs=Date.now();
        await this.cleanup(catalog);
        await this.state.storage.put("catalog",catalog);
        await this.drainDeletes(catalog,prefix);
        return json({status:"ok",projectId:id,complete:true,
          verifiedParts:p.partCount,verifiedBytes:p.sizeBytes});
      }
      return error(404,"proje işlemi bulunamadı");
    }catch(e){
      if(e instanceof SyntaxError||e instanceof RangeError||
         String(e?.message||"").includes("invalid")||
         String(e?.message||"").includes("duplicate"))
        return error(400,"geçersiz veya büyük veri");
      return error(503,"proje depolama işlemi tamamlanamadı");
    }
  }
  async cleanup(catalog){
    const all=Object.values(catalog.projects);
    const plan=planProjectRetention({
      completedProjects:all.filter(p=>p.status==="complete").map(p=>({
        id:p.id,completedAtMs:p.completedAtMs})),
      openProjectIds:catalog.openProjectIds
    });
    const remove=new Set(plan.deleteProjectIds);
    // Closed partial uploads are not usable backups; can be discarded.
    for(const p of all)if(p.status!=="complete"&&
      !catalog.openProjectIds.includes(p.id))remove.add(p.id);
    for(const id of remove){
      const p=catalog.projects[id];
      if(!p)continue;
      catalog.pendingDeletes.push("projects/"+p.id+"/"+p.uploadId+"/parts/");
      delete catalog.projects[id];
    }
  }
  async drainDeletes(catalog,prefix){
    if(!Array.isArray(catalog.pendingDeletes)||!catalog.pendingDeletes.length)return;
    const remains=[];
    for(const keyPrefix of catalog.pendingDeletes){
      try{
        let cursor;
        do{
          const listing=await this.env.MUSACAD_PROJECT_BUCKET.list({
            prefix:prefix+keyPrefix,limit:1000,...(cursor?{cursor}:{})
          });
          if(listing.objects?.length)
            await this.env.MUSACAD_PROJECT_BUCKET.delete(listing.objects.map(x=>x.key));
          cursor=listing.truncated?listing.cursor:null;
        }while(cursor);
      }catch(_){remains.push(keyPrefix);}
    }
    catalog.pendingDeletes=remains;
    await this.state.storage.put("catalog",catalog);
  }
}
