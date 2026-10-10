import test from "node:test";
import assert from "node:assert/strict";
import { generateKeyPairSync, sign, webcrypto, createHash } from "node:crypto";
import worker, { MusaCadProjectCoordinator } from "./src/index.js";

if(!globalThis.crypto)globalThis.crypto=webcrypto;
const A="00000000-0000-4000-8000-00000000000a";
const B="00000000-0000-4000-8000-00000000000b";
const C="00000000-0000-4000-8000-00000000000c";
const D="00000000-0000-4000-8000-00000000000d";
const DEVICE="MC-12345678-90ABCDEF-12345678";
const DEVICE2="MC-22222222-33333333-44444444";
const hex=x=>createHash("sha256").update(x).digest("hex");
const jwt=(privateKey,device=DEVICE)=>{
  const body=Buffer.from("MAI1|"+device+"|"+(Date.now()+600000));
  const signature=sign("RSA-SHA256",body,privateKey);
  return "MAI1."+body.toString("base64url")+"."+signature.toString("base64url");
};
class Bucket {
  constructor(){this.objects=new Map();}
  async put(key,contents){this.objects.set(key,Buffer.from(contents));}
  async list({prefix,limit=1000,cursor}){
    const names=[...this.objects.keys()].filter(k=>k.startsWith(prefix)).sort();
    const from=cursor?names.findIndex(k=>k===cursor)+1:0;
    const slice=names.slice(from,from+limit);
    return {objects:slice.map(key=>({key})),truncated:from+limit<names.length,
      cursor:slice.length?slice[slice.length-1]:""};
  }
  async delete(keys){for(const key of(Array.isArray(keys)?keys:[keys]))this.objects.delete(key);}
}
function setup(){
  const {privateKey,publicKey}=generateKeyPairSync("rsa",{modulusLength:2048});
  const publicPem=publicKey.export({type:"spki",format:"pem"}).toString();
  const instances=new Map(),bucket=new Bucket();
  const env={MUSACAD_AUTOUPLOAD_ENABLED:"true",MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:publicPem,
    MUSACAD_PROJECT_BUCKET:bucket};
  env.MUSACAD_PROJECT_COORDINATOR={
    idFromName:name=>name,
    get:name=>{
      if(!instances.has(name)){
        const data=new Map();
        const state={storage:{
          get:async key=>data.get(key),
          put:async(key,value)=>data.set(key,structuredClone(value))
        }};
        instances.set(name,new MusaCadProjectCoordinator(state,env));
      }
      return instances.get(name);
    }
  };
  const send=async(path,{method="POST",data,headers={},device=DEVICE,authenticated=true}={})=>{
    const body=data instanceof Uint8Array?data:data===undefined?undefined:JSON.stringify(data);
    const res=await worker.fetch(new Request("https://ai.musacad.test"+path,{
      method,headers:{...(authenticated?{authorization:"Bearer "+jwt(privateKey,device)}:{}),
        ...(body&&!(data instanceof Uint8Array)?{"content-type":"application/json"}:{}),...headers},
      ...(body!==undefined?{body}:{})
    }),env);
    return {code:res.status,body:await res.json()};
  };
  const sync=(ids,revision=1,device=DEVICE)=>send("/v1/projects/sync-open",{data:{openProjectIds:ids,revision},device});
  const upload=async(id,data,revision=1,device=DEVICE)=>{
    const bytes=Buffer.from(data);
    const meta=await send("/v1/projects/init",{device,data:{
      projectId:id,fileName:"proje.dwg",sizeBytes:bytes.length,sha256:hex(bytes)}});
    if(meta.code!==200)return meta;
    const step=meta.body.chunkBytes;
    for(let part=0;part<Math.ceil(bytes.length/step);part++){
      const piece=bytes.subarray(part*step,Math.min(bytes.length,(part+1)*step));
      const r=await send("/v1/projects/"+id+"/chunks/"+part,{
        method:"PUT",device,data:new Uint8Array(piece),headers:{"x-chunk-sha256":hex(piece)}});
      if(r.code!==200)return r;
    }
    return send("/v1/projects/"+id+"/complete",{device});
  };
  return {env,bucket,send,sync,upload};
}

test("without private bucket or opt-in project API fails closed",async()=>{
  const {env,send}=setup();
  delete env.MUSACAD_PROJECT_BUCKET;
  assert.equal((await send("/v1/projects/status",{method:"GET"})).code,503);
});
test("unsigned project API cannot access device storage",async()=>{
  const {send}=setup();
  assert.equal((await send("/v1/projects/status",{method:"GET",authenticated:false})).code,401);
});
test("open A B C; close B; keep A and C remotely",async()=>{
  const s=setup();
  assert.equal((await s.sync([A,B,C])).code,200);
  for(const [id,data] of [[A,"mechanical"],[B,"architectural"],[C,"electrical"]]){
    assert.equal((await s.upload(id,data)).code,200);
  }
  assert.equal(s.bucket.objects.size,3);
  assert.equal((await s.sync([A,C],2)).code,200);
  const status=await s.send("/v1/projects/status",{method:"GET"});
  assert.deepEqual(status.body.projects.map(p=>p.id).sort(),[A,C]);
  assert.equal(s.bucket.objects.size,2);
});
test("new D failing upload does not destroy final verified A; confirmed D removes A",async()=>{
  const s=setup();
  await s.sync([A]);assert.equal((await s.upload(A,"old")).code,200);
  await s.sync([D],2);
  assert.equal(s.bucket.objects.size,1);
  assert.equal((await s.upload(D,"new")).code,200);
  const status=await s.send("/v1/projects/status",{method:"GET"});
  assert.deepEqual(status.body.projects.map(p=>p.id),[D]);
  assert.equal(s.bucket.objects.size,1);
});
test("zero tabs retains newest completed project",async()=>{
  const s=setup();
  await s.sync([A,B]);await s.upload(A,"first");await s.upload(B,"second");
  await s.sync([],2);
  const st=await s.send("/v1/projects/status",{method:"GET"});
  assert.deepEqual(st.body.projects.map(p=>p.id),[B]);
});
test("digest mismatch never counts as received or completes",async()=>{
  const s=setup();await s.sync([A]);
  const bytes=Buffer.from("test-data");
  assert.equal((await s.send("/v1/projects/init",{data:{
    projectId:A,fileName:"test.dwg",sizeBytes:bytes.length,sha256:hex(bytes)}})).code,200);
  assert.equal((await s.send("/v1/projects/"+A+"/chunks/0",{method:"PUT",
    data:new Uint8Array(bytes),headers:{"x-chunk-sha256":hex("wrong")}})).code,422);
  assert.equal((await s.send("/v1/projects/"+A+"/complete")).code,409);
});
test("missing final chunk blocks completion, client may resume",async()=>{
  const s=setup();await s.sync([A]);
  const bytes=Buffer.alloc(1024*1024+5,17);
  assert.equal((await s.send("/v1/projects/init",{data:{
    projectId:A,fileName:"heavy.dwg",sizeBytes:bytes.length,sha256:hex(bytes)}})).code,200);
  const first=bytes.subarray(0,1024*1024);
  assert.equal((await s.send("/v1/projects/"+A+"/chunks/0",{method:"PUT",
    data:new Uint8Array(first),headers:{"x-chunk-sha256":hex(first)}})).code,200);
  assert.equal((await s.send("/v1/projects/"+A+"/complete")).code,409);
  const init=await s.send("/v1/projects/init",{data:{
    projectId:A,fileName:"heavy.dwg",sizeBytes:bytes.length,sha256:hex(bytes)}});
  assert.deepEqual(init.body.receivedParts,[0]);
  assert.equal((await s.send("/v1/projects/"+A+"/chunks/1",{method:"PUT",
    data:new Uint8Array(bytes.subarray(1024*1024)),
    headers:{"x-chunk-sha256":hex(bytes.subarray(1024*1024))}})).code,200);
  assert.equal((await s.send("/v1/projects/"+A+"/complete")).code,200);
});
test("late tab revision cannot delete newly open project",async()=>{
  const s=setup();await s.sync([A],6);await s.upload(A,"original");
  assert.equal((await s.sync([],5)).code,409);
  const st=await s.send("/v1/projects/status",{method:"GET"});
  assert.deepEqual(st.body.openProjectIds,[A]);
  assert.equal(st.body.projects.length,1);
});
test("two signed devices cannot see each other's projects",async()=>{
  const s=setup();await s.sync([A],1,DEVICE);await s.upload(A,"private",1,DEVICE);
  const other=await s.send("/v1/projects/status",{method:"GET",device:DEVICE2});
  assert.equal(other.body.projects.length,0);
  assert.equal((await s.send("/v1/projects/"+A+"/complete",{device:DEVICE2})).code,404);
});
test("invalid UUID and excessive tabs rejected",async()=>{
  const s=setup();
  assert.equal((await s.sync([A,B,C,D,"00000000-0000-4000-8000-00000000000e"])).code,400);
  assert.equal((await s.sync([A,A])).code,400);
  assert.equal((await s.send("/v1/projects/init",{data:{
    projectId:"../override",fileName:"bad.dwg",sizeBytes:2,sha256:hex("hi")}})).code,400);
});
