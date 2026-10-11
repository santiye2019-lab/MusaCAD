// Bounded ASCII DXF entity extraction for engineering evidence.
// This is NOT a DWG binary decoder. Unsupported inputs fail closed.
export function extractCadEvidence(bytes,{maxEntities=5000}={}){
  if(!(bytes instanceof Uint8Array)||bytes.length>32*1024*1024)
    throw new Error("unsupported_cad_size");
  if(bytes.length>=6&&String.fromCharCode(...bytes.slice(0,6))==="AutoCA")
    throw new Error("binary_dxf_unsupported");
  if(bytes.length>=6&&String.fromCharCode(...bytes.slice(0,6))==="AC10")
    throw new Error("binary_dwg_decoder_required");
  const raw=new TextDecoder("utf-8",{fatal:true}).decode(bytes);
  if(!raw.includes("SECTION")||!raw.includes("ENTITIES"))
    throw new Error("not_ascii_dxf");
  const lines=raw.replace(/\r/g,"").split("\n");
  const pairs=[];
  for(let i=0;i+1<lines.length;i+=2){
    const code=Number(lines[i].trim());
    if(!Number.isInteger(code))throw new Error("invalid_dxf_group");
    pairs.push([code,lines[i+1].trim()]);
  }
  let section="",entity=null,inEntities=false,seen=0;
  const entities=[],layers=new Set(),layouts=new Set();
  const supported=new Set(["LINE","LWPOLYLINE","POLYLINE","CIRCLE","ARC","INSERT","TEXT","MTEXT","DIMENSION"]);
  function flush(){
    if(!entity)return;
    seen++;
    if(entities.length<maxEntities&&supported.has(entity.type)){
      entities.push(entity);
      if(entity.layer)layers.add(entity.layer);
      if(entity.layout)layouts.add(entity.layout);
    }
    entity=null;
  }
  for(let i=0;i<pairs.length;i++){
    const [code,value]=pairs[i];
    if(code===0&&value==="SECTION"){flush();section="";continue;}
    if(code===2&&!section){section=value;inEntities=section==="ENTITIES";continue;}
    if(code===0&&value==="ENDSEC"){flush();section="";inEntities=false;continue;}
    if(!inEntities)continue;
    if(code===0){flush();entity={type:value};continue;}
    if(!entity)continue;
    if(code===8)entity.layer=value.slice(0,100);
    else if(code===410)entity.layout=value.slice(0,100);
    else if(code===1||code===3)entity.text=(entity.text||"")+value.slice(0,500);
    else if(code===2)entity.block=value.slice(0,100);
    else if([10,20,30,11,21,31,40,50].includes(code)){
      const n=Number(value);if(Number.isFinite(n))entity["g"+code]=n;
    }
  }
  flush();
  return {format:"ASCII_DXF",totalEntities:seen,sampledEntities:entities.length,
    coverageComplete:seen<=maxEntities,layers:[...layers],layouts:[...layouts],
    entities,limitations:["Only ASCII DXF ENTITIES extracted","No geometric clash verification","No DWG rendering or image analysis"]};
}
