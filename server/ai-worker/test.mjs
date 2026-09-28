import test from "node:test";
import assert from "node:assert/strict";
import { generateKeyPairSync, sign, webcrypto } from "node:crypto";
import worker, { parseOpenAiOutput, cadProposalTools } from "./src/index.js";

if (!globalThis.crypto) globalThis.crypto = webcrypto;

const DEVICE="MC-12345678-90ABCDEF-12345678";

function base64url(buf){
  return Buffer.from(buf).toString("base64").replace(/\+/g,"-").replace(/\//g,"_").replace(/=+$/g,"");
}

function sessionPair(){
  const {privateKey,publicKey}=generateKeyPairSync("rsa",{modulusLength:2048});
  return {
    privateKey,
    publicPem:publicKey.export({type:"spki",format:"pem"}).toString()
  };
}

function sessionToken(privateKey,expiresAtMs){
  const payload=Buffer.from(`MAI1|${DEVICE}|${expiresAtMs}`,"utf8");
  const signature=sign("RSA-SHA256",payload,privateKey);
  return `MAI1.${base64url(payload)}.${base64url(signature)}`;
}

test("OpenAI output parser keeps answer, web usage and CAD proposals",()=>{
  const parsed=parseOpenAiOutput({
    output:[
      {type:"message",content:[{type:"output_text",text:"Pis su hattında iki kontrol adayı var."}]},
      {type:"web_search_call",id:"ws_1",action:{sources:[{title:"Kaynak A",url:"https://example.com/a"}]}},
      {type:"function_call",name:"cad_change_layer",arguments:JSON.stringify({sourceId:12,layer:"PIS_SU",reason:"Yanlış katman"})}
    ]
  });
  assert.equal(parsed.reply,"Pis su hattında iki kontrol adayı var.");
  assert.equal(parsed.webUsed,true);
  assert.equal(parsed.sources.length,1);
  assert.equal(parsed.sources[0].url,"https://example.com/a");
  assert.equal(parsed.actions.length,1);
  assert.equal(parsed.actions[0].name,"cad_change_layer");
  assert.equal(parsed.actions[0].arguments.sourceId,12);
});

test("CAD proposal tools are strict and proposal-only surface is bounded",()=>{
  const tools=cadProposalTools();
  assert.ok(tools.length>=6);
  const names=new Set(tools.map(t=>t.name));
  assert.ok(names.has("cad_move_entity"));
  assert.ok(names.has("cad_add_line"));
  assert.ok(names.has("cad_replace_text"));
  for(const tool of tools){
    assert.equal(tool.type,"function");
    assert.equal(tool.strict,true);
    assert.equal(tool.parameters.additionalProperties,false);
  }
});

test("analyze endpoint rejects missing AI session before contacting OpenAI",async()=>{
  let called=false;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{"content-type":"application/json"},
    body:JSON.stringify({prompt:"incele",cad:{schema:"musacad-cad-json/v1"}})
  });
  const response=await worker.fetch(request,{
    OPENAI_API_KEY:"server-secret",
    OPENAI_MODEL:"test-model",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:"not-used-for-missing-auth",
    __fetch:async()=>{called=true;throw new Error("must not call")}
  });
  assert.equal(response.status,401);
  assert.equal(called,false);
});

test("valid signed session reaches Responses API and returns proposed actions",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(url,options)=>{
    assert.equal(String(url),"https://api.openai.com/v1/responses");
    assert.equal(options.headers.authorization,"Bearer server-secret");
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[
        {type:"message",content:[{type:"output_text",text:"Kontrol raporu hazır."}]},
        {type:"function_call",name:"cad_add_text",arguments:JSON.stringify({x:10,y:20,text:"KONTROL",layer:"NOTLAR",reason:"Revizyon notu"})}
      ]
    }),{status:200,headers:{"content-type":"application/json"}});
  };

  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{
      authorization:"Bearer "+token,
      "content-type":"application/json"
    },
    body:JSON.stringify({
      prompt:"Güncel kaynaklarla kontrol et ve gerekli notu eklemeyi öner",
      allowWeb:true,
      allowEditProposals:true,
      cad:{
        schema:"musacad-cad-json/v1",
        fileName:"mekanik.dwg",
        cloudPolicy:{rawDrawingIncluded:false,automaticEditsAllowed:false,editActionsRequireUserApproval:true},
        items:[]
      }
    })
  });

  const response=await worker.fetch(request,{
    OPENAI_API_KEY:"server-secret",
    OPENAI_MODEL:"test-model",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:fetcher
  });
  const body=await response.json();
  assert.equal(response.status,200);
  assert.equal(body.status,"ok");
  assert.equal(body.reply,"Kontrol raporu hazır.");
  assert.equal(body.actions.length,1);
  assert.equal(body.actions[0].name,"cad_add_text");
  assert.ok(upstreamBody.tools.some(t=>t.type==="web_search"));
  assert.ok(upstreamBody.tools.some(t=>t.name==="cad_add_text"));
  assert.equal(upstreamBody.store,false);
  assert.deepEqual(upstreamBody.include,["web_search_call.action.sources"]);
  assert.equal(upstreamBody.instructions.includes("PROPOSALS ONLY"),true);
});
