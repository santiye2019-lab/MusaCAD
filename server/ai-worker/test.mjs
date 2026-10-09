test("Gemini quota errors never claim background automatic recovery",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+600000);
  const response=await worker.fetch(new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({prompt:"Projeyi incele",cad:{schema:"musacad-cad-json/v1",items:[]}})
  }),{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>new Response(JSON.stringify({error:{message:"limit"}}),{status:429})
  });
  const body=await response.json();
  assert.equal(response.status,429);
  assert.match(body.message,/yenilendiğinde yeniden deneyin/);
  assert.equal(body.message.includes("otomatik olarak yeniden çalışır"),false);
});

test("AI provider timeout returns bounded error instead of hanging",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({prompt:"Projeyi derin analiz et",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const started=Date.now();
  const response=await worker.fetch(request,{
    AI_PROVIDER:"gemini",
    GEMINI_API_KEY:"dummy",
    GEMINI_MODEL:"test",
    AI_UPSTREAM_TIMEOUT_MS:"60",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>new Promise(()=>{})
  });
  const body=await response.json();
  assert.equal(response.status,504);
  assert.equal(body.status,"timeout");
  assert.match(body.message,/bekleme süresini aştı|yanıtı bekleme/);
  assert.ok(Date.now()-started<5000);
});

import test from "node:test";
import assert from "node:assert/strict";
import { generateKeyPairSync, sign, webcrypto } from "node:crypto";
import worker, { parseOpenAiOutput, parseGeminiChatOutput, cadProposalTools } from "./src/index.js";

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

function developerSessionToken(privateKey,expiresAtMs){
  const payload=Buffer.from(`MAI2|${DEVICE}|${expiresAtMs}|developer`,"utf8");
  const signature=sign("RSA-SHA256",payload,privateKey);
  return `MAI2.${base64url(payload)}.${base64url(signature)}`;
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
  assert.ok(tools.length>=13);
  const names=new Set(tools.map(t=>t.name));
  assert.ok(names.has("cad_move_entity"));
  assert.ok(names.has("cad_add_line"));
  assert.ok(names.has("cad_replace_text"));
  assert.ok(names.has("cad_add_polyline"));
  assert.ok(names.has("cad_offset_entity"));
  assert.ok(names.has("cad_trim_line"));
  assert.ok(names.has("cad_extend_line"));
  assert.ok(names.has("cad_continue_path"));
  assert.ok(names.has("cad_add_pipe_note"));
  assert.ok(names.has("cad_insert_mechanical_block"));
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
      previousChat:"Kullanıcı: Pompa sayısı kaç?\\nGandalf: Çizimde 2 pompa adayı.",
      localEvidence:"Yerel ön metraj: 2 pompa bloğu; kaynak doğrulaması gerekli.",
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
  assert.equal(upstreamBody.instructions.includes("vertices"),true);
  assert.ok(upstreamBody.input[0].content.includes("PRIOR CONVERSATION"));
  assert.ok(upstreamBody.input[0].content.includes("Yerel ön metraj: 2 pompa bloğu"));
  assert.ok(upstreamBody.instructions.includes("paraphrases"));

  assert.ok(upstreamBody.tools.some(t=>t.name==="cad_trim_line"));
  assert.ok(upstreamBody.tools.some(t=>t.name==="cad_insert_mechanical_block"));
});


test("MAI2 developer session is accepted and reported as developer mode",async()=>{
  const keys=sessionPair();
  const token=developerSessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(_url,options)=>{
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[{type:"message",content:[{type:"output_text",text:"Geliştirici analizi hazır."}]}]
    }),{status:200,headers:{"content-type":"application/json"}});
  };

  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Projeyi derin analiz et",
      allowWeb:false,
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
  assert.equal(body.accessMode,"developer");
  assert.equal(upstreamBody.instructions.includes("Current access mode: developer"),true);
  assert.equal(upstreamBody.instructions.includes("explicit user approval"),true);
});


test("MEKAI expert profile adds trusted mechanical instructions",async()=>{
  const keys=sessionPair();
  const token=developerSessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(_url,options)=>{
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[{type:"message",content:[{type:"output_text",text:"Yangın uzman raporu hazır."}]}]
    }),{status:200,headers:{"content-type":"application/json"}});
  };

  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"GMEKAI_FIRE projeyi incele",
      expertProfile:"fire",
      allowWeb:false,
      allowEditProposals:true,
      cad:{
        schema:"musacad-cad-json/v1",
        fileName:"yangin.dwg",
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
  assert.equal(body.expertProfile,"fire");
  assert.equal(upstreamBody.instructions.includes("MEKAI fire-protection expert profile"),true);
  assert.equal(upstreamBody.instructions.includes("fire-department connection"),true);
});

test("multi-discipline expert profile adds trusted structural instructions",async()=>{
  const keys=sessionPair();
  const token=developerSessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(_url,options)=>{
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[{type:"message",content:[{type:"output_text",text:"Statik rezervasyon raporu hazır."}]}]
    }),{status:200,headers:{"content-type":"application/json"}});
  };

  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"GSTATIKAI_OPENINGS taşıyıcı sistem rezervasyonlarını incele",
      expertProfile:"structural_openings",
      allowWeb:false,
      allowEditProposals:true,
      cad:{
        schema:"musacad-cad-json/v1",
        fileName:"statik.dwg",
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
  assert.equal(body.expertProfile,"structural_openings");
  assert.equal(upstreamBody.instructions.includes("STATIKAI opening/reservation profile"),true);
  assert.equal(upstreamBody.instructions.includes("Never conclude structural adequacy"),false);
  assert.equal(upstreamBody.instructions.includes("never recommend field drilling without structural approval"),true);
});

test("multi-discipline expert profile adds trusted electrical instructions",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(_url,options)=>{
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[{type:"message",content:[{type:"output_text",text:"Elektrik raporu hazır."}]}]
    }),{status:200,headers:{"content-type":"application/json"}});
  };
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"GELKAI_POWER kontrol et",
      expertProfile:"electrical_power",
      cad:{schema:"musacad-cad-json/v1",cloudPolicy:{rawDrawingIncluded:false},items:[]}
    })
  });
  const response=await worker.fetch(request,{
    OPENAI_API_KEY:"server-secret",
    OPENAI_MODEL:"test-model",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:fetcher
  });
  const body=await response.json();
  assert.equal(body.expertProfile,"electrical_power");
  assert.equal(upstreamBody.instructions.includes("ELKAI power profile"),true);
  assert.equal(upstreamBody.instructions.includes("Do not infer cable sizing"),true);
});

test("project package is read-only and sent as multi-drawing context",async()=>{
  const keys=sessionPair();
  const token=developerSessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(_url,options)=>{
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[{type:"message",content:[{type:"output_text",text:"Çok disiplinli paket analizi hazır."}]}]
    }),{status:200,headers:{"content-type":"application/json"}});
  };
  const one=(name,layer)=>({
    fileName:name,
    detectedDiscipline:layer==="S_KIRIS"?"STRUCTURAL":"MECHANICAL",
    cad:{
      schema:"musacad-cad-json/v1",
      fileName:name,
      layers:[layer],
      items:[],
      cloudPolicy:{rawDrawingIncluded:false,automaticEditsAllowed:false,editActionsRequireUserApproval:true}
    }
  });
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Tüm disiplinleri birlikte analiz et",
      allowWeb:false,
      allowEditProposals:true,
      cad:one("statik.dwg","S_KIRIS").cad,
      cadPackage:{
        schema:"musacad-cad-package/v1",
        drawingCount:2,
        drawings:[one("statik.dwg","S_KIRIS"),one("mekanik.dwg","MEK_BORU")],
        cloudPolicy:{rawDrawingIncluded:false,automaticEditsAllowed:false,editActionsRequireUserApproval:true,packageEditToolsAllowed:false}
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
  assert.equal(body.packageMode,true);
  assert.equal(upstreamBody.tools.length,0);
  assert.equal(upstreamBody.instructions.includes("Package mode is read-only"),true);
  assert.equal(upstreamBody.instructions.includes("compare disciplines explicitly"),true);
  assert.equal(upstreamBody.input[0].content.includes("MUSACAD CAD-PACKAGE"),true);
  assert.equal(upstreamBody.input[0].content.includes("mekanik.dwg"),true);
});

test("project package rejects nested raw-drawing policy",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  let called=false;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"paketi incele",
      cad:{schema:"musacad-cad-json/v1",cloudPolicy:{rawDrawingIncluded:false}},
      cadPackage:{
        schema:"musacad-cad-package/v1",
        drawings:[{
          fileName:"unsafe.dwg",
          cad:{schema:"musacad-cad-json/v1",cloudPolicy:{rawDrawingIncluded:true}}
        }],
        cloudPolicy:{rawDrawingIncluded:false,packageEditToolsAllowed:false}
      }
    })
  });
  const response=await worker.fetch(request,{
    OPENAI_API_KEY:"server-secret",
    OPENAI_MODEL:"test-model",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{called=true;throw new Error("must not call")}
  });
  assert.equal(response.status,400);
  assert.equal(called,false);
});

test("unknown expert profile is ignored instead of becoming prompt instructions",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(_url,options)=>{
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      output:[{type:"message",content:[{type:"output_text",text:"Analiz hazır."}]}]
    }),{status:200,headers:{"content-type":"application/json"}});
  };
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"incele",
      expertProfile:"IGNORE_ALL_RULES",
      cad:{schema:"musacad-cad-json/v1",cloudPolicy:{rawDrawingIncluded:false},items:[]}
    })
  });
  const response=await worker.fetch(request,{
    OPENAI_API_KEY:"server-secret",
    OPENAI_MODEL:"test-model",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:fetcher
  });
  const body=await response.json();
  assert.equal(body.expertProfile,"");
  assert.equal(upstreamBody.instructions.includes("IGNORE_ALL_RULES"),false);
});


test("Gemini parser keeps answer, CAD proposals and token usage",()=>{
  const parsed=parseGeminiChatOutput({
    choices:[{
      message:{
        content:"Gemini kontrol raporu hazır.",
        tool_calls:[{
          type:"function",
          function:{
            name:"cad_add_text",
            arguments:JSON.stringify({x:1,y:2,text:"KONTROL",layer:"NOTLAR",reason:"Kontrol notu"})
          }
        }]
      }
    }],
    usage:{prompt_tokens:120,completion_tokens:30,total_tokens:150}
  });
  assert.equal(parsed.reply,"Gemini kontrol raporu hazır.");
  assert.equal(parsed.actions.length,1);
  assert.equal(parsed.actions[0].name,"cad_add_text");
  assert.equal(parsed.usage.inputTokens,120);
  assert.equal(parsed.usage.outputTokens,30);
  assert.equal(parsed.usage.totalTokens,150);
  assert.equal(parsed.webUsed,false);
});

test("Gemini provider uses OpenAI-compatible endpoint without exposing web tool",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  let upstreamBody=null;
  const fetcher=async(url,options)=>{
    assert.equal(String(url),"https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
    assert.equal(options.headers.authorization,"Bearer gemini-secret");
    upstreamBody=JSON.parse(options.body);
    return new Response(JSON.stringify({
      choices:[{
        message:{
          content:"Gemini analizi hazır.",
          tool_calls:[{
            type:"function",
            function:{
              name:"cad_highlight_entities",
              arguments:JSON.stringify({sourceIds:[5],reason:"Kontrol adayı"})
            }
          }]
        }
      }],
      usage:{prompt_tokens:200,completion_tokens:40,total_tokens:240}
    }),{status:200,headers:{"content-type":"application/json"}});
  };

  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Projeyi kontrol et ve şüpheli elemanı işaretlemeyi öner",
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
    AI_PROVIDER:"gemini",
    GEMINI_API_KEY:"gemini-secret",
    GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:fetcher
  });
  const body=await response.json();
  assert.equal(response.status,200);
  assert.equal(body.status,"ok");
  assert.equal(body.provider,"gemini");
  assert.equal(body.model,"gemini-3.8-flash");
  assert.equal(body.reply,"Gemini analizi hazır.");
  assert.equal(body.actions.length,1);
  assert.equal(body.usage.totalTokens,240);
  assert.equal(upstreamBody.model,"gemini-3.8-flash");
  assert.ok(Array.isArray(upstreamBody.messages));
  assert.equal(upstreamBody.messages[0].role,"system");
  assert.equal(upstreamBody.messages[0].content.includes("no live web-search tool"),true);
  assert.ok(Array.isArray(upstreamBody.tools));
  assert.ok(upstreamBody.tools.some(t=>t.function&&t.function.name==="cad_highlight_entities"));
  assert.equal(upstreamBody.tools.some(t=>t.type==="web_search"),false);
});

test("Gemini quota exhaustion returns a clear free-tier message",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+10*60*1000);
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"incele",
      cad:{schema:"musacad-cad-json/v1",cloudPolicy:{rawDrawingIncluded:false},items:[]}
    })
  });
  const response=await worker.fetch(request,{
    AI_PROVIDER:"gemini",
    GEMINI_API_KEY:"gemini-secret",
    GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>new Response(JSON.stringify({error:{message:"quota exceeded"}}),{status:429,headers:{"content-type":"application/json"}})
  });
  const body=await response.json();
  assert.equal(response.status,429);
  assert.equal(body.status,"quota_exhausted");
  assert.match(body.message,/kotas[ıi]|hız sınırı/);
});


test("Gemini receives consented visual CAD regions together with vector evidence and discipline scope",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+600000);
  const jpeg="/9j/"+("A".repeat(120));
  let sent=null, endpoint="";
  const req=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Sıhhi tesisat olarak analiz et",
      analysisScope:"sanitary",
      cad:{schema:"musacad-cad-json/v1",items:[{sourceId:7,type:"LINE",layer:"SIHHI",length:12}]},
      visualEvidence:{
        schema:"musacad-visual-evidence/v1",
        rawDrawingIncluded:false,complete:false,
        images:[{
          mime:"image/jpeg",base64:jpeg,label:"full-sheet-overview",
          width:800,height:800,contentBounds:[0,0,100,100],drawingBounds:[1000,2000,1100,2100]
        }]
      }
    })
  });
  const response=await worker.fetch(req,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-test",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(url,init)=>{
      endpoint=String(url);
      sent=JSON.parse(init.body);
      return new Response(JSON.stringify({
        choices:[{message:{role:"assistant",content:"Çizimde görülen boru güzergâhı için çap teyidi gerekli."}}]
      }),{status:200,headers:{"content-type":"application/json"}});
    }
  });
  assert.equal(response.status,200);
  const data=await response.json();
  assert.equal(data.status,"ok");
  assert.equal(data.analysisScope,"sanitary");
  assert.equal(data.visualRegionCount,1);
  assert.equal(data.visualCoverageComplete,false);
  assert.match(endpoint,/generativelanguage\.googleapis\.com/);
  assert.equal(sent.messages[1].content[0].type,"text");
  assert.match(sent.messages[1].content[0].text,/SIHHI/);
  assert.match(sent.messages[0].content,/primary discipline scope: sanitary/);
  assert.match(sent.messages[1].content[0].text,/drawingBounds/);
  assert.equal(sent.messages[1].content[1].type,"image_url");
  assert.equal(sent.messages[1].content[1].image_url.url,"data:image/jpeg;base64,"+jpeg);
});

test("OpenAI multimodal route constructs input_image with the same CAD evidence",async()=>{
  const keys=sessionPair();
  const req=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Statik paftayı analiz et",analysisScope:"structural",
      cad:{schema:"musacad-cad-json/v1",items:[]},
      visualEvidence:{schema:"musacad-visual-evidence/v1",rawDrawingIncluded:false,complete:true,
        images:[{mime:"image/jpeg",base64:"/9j/"+("A".repeat(120)),label:"full-sheet-overview",
          width:800,height:800,contentBounds:[0,0,200,200],drawingBounds:[0,0,200,200]}]}
    })
  });
  let sent;
  const res=await worker.fetch(req,{
    OPENAI_API_KEY:"dummy",OPENAI_MODEL:"test",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(url,init)=>{
      assert.equal(String(url),"https://api.openai.com/v1/responses");
      sent=JSON.parse(init.body);
      return new Response(JSON.stringify({output:[{type:"message",content:[{type:"output_text",text:"Statik görsel ön inceleme."}]}]}),
        {status:200,headers:{"content-type":"application/json"}});
    }
  });
  assert.equal(res.status,200);
  assert.equal(sent.input[0].content[1].type,"input_image");
  assert.equal(sent.input[0].content[0].type,"input_text");
  assert.match(sent.instructions,/primary discipline scope: structural/);
});

test("Malformed or unauthorized visual payload is rejected before provider call",async()=>{
  const keys=sessionPair();
  let invoked=false;
  const body={prompt:"projeyi analiz et",cad:{schema:"musacad-cad-json/v1",items:[]},
    visualEvidence:{schema:"musacad-visual-evidence/v1",rawDrawingIncluded:false,
      images:[{mime:"image/png",base64:"AAA",label:"full-sheet-overview",
        width:600,height:600,contentBounds:[0,0,10,10]}]}};
  const req=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},body:JSON.stringify(body)});
  const res=await worker.fetch(req,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"test",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{invoked=true;throw new Error("provider must not be called")}
  });
  assert.equal(res.status,400);
  assert.equal(invoked,false);
});


test("last 3x3 sweep batch with one detailed tile sends mapped vision to Gemini",async()=>{
  const keys=sessionPair();
  const jpeg="/9j/"+("A".repeat(120));
  const img=(label,from,to)=>({
    mime:"image/jpeg",base64:jpeg,label,width:1200,height:1200,
    contentBounds:[from,from,to,to],drawingBounds:[1000+from,2000+from,1000+to,2000+to]
  });
  let sent;
  const req=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{
      authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Projeyi analiz et",analysisScope:"all",
      cad:{schema:"musacad-cad-json/v1",items:[{sourceId:41,type:"TEXT",layer:"PIS_SU",text:"DN100",centerX:1250,centerY:2250}]},
      visualEvidence:{
        schema:"musacad-visual-evidence/v1",sweepSchema:"musacad-visual-sweep/v1",
        sweepBatch:3,sweepBatchCount:3,totalDetailedTiles:9,firstTile:9,lastTile:9,
        rawDrawingIncluded:false,complete:true,
        images:[img("full-sheet-overview",0,300),img("sheet-tile-9",200,300)]
      }
    })
  });
  const response=await worker.fetch(req,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"stub",GEMINI_MODEL:"gemini-test",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(url,init)=>{
      sent=JSON.parse(init.body);
      return new Response(JSON.stringify({choices:[{message:{content:"sheet-tile-9: DN100 görünüyor."}}]}),
        {status:200,headers:{"content-type":"application/json"}});
    }
  });
  assert.equal(response.status,200);
  const output=await response.json();
  assert.equal(output.status,"ok");
  assert.equal(output.visualRegionCount,2);
  assert.equal(output.visualCoverageComplete,true);
  assert.match(sent.messages[1].content[0].text,/sheet-tile-9/);
  assert.match(sent.messages[1].content[0].text,/VISUAL SWEEP BATCH: 3/);
  assert.match(sent.messages[1].content[0].text,/CO-LOCATED SOURCE CANDIDATES/);
  assert.match(sent.messages[1].content[0].text,/"sourceId":41/);
  assert.equal(sent.messages[1].content[2].type,"image_url");
});

test("sweep rejects skipped tile identity or fabricated full coverage before provider",async()=>{
  const keys=sessionPair();
  const jpeg="/9j/"+("A".repeat(120));
  const frame=label=>({
    mime:"image/jpeg",base64:jpeg,label,width:640,height:640,
    contentBounds:[0,0,100,100],drawingBounds:[0,0,100,100]
  });
  for(const [labels,complete] of [
    [["full-sheet-overview","sheet-tile-2"],false],
    [["full-sheet-overview","sheet-tile-1"],true],
    [["full-sheet-overview","sheet-tile-1","sheet-tile-1"],false]
  ]){
    let upstream=false;
    const req=new Request("https://ai.musacad.test/v1/analyze",{
      method:"POST",headers:{
        authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
        "content-type":"application/json"},
      body:JSON.stringify({
        prompt:"Projeyi analiz et",cad:{schema:"musacad-cad-json/v1",items:[]},
        visualEvidence:{schema:"musacad-visual-evidence/v1",sweepSchema:"musacad-visual-sweep/v1",
          sweepBatch:1,sweepBatchCount:3,totalDetailedTiles:9,firstTile:1,lastTile:4,
          complete,rawDrawingIncluded:false,images:labels.map(frame)}
      })
    });
    const res=await worker.fetch(req,{
      AI_PROVIDER:"gemini",GEMINI_API_KEY:"stub",GEMINI_MODEL:"test",
      MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
      __fetch:async()=>{upstream=true;throw Error("must not call upstream");}
    });
    assert.equal(res.status,400);
    assert.equal(upstream,false);
  }
});


test("Gandalf view-focus images keep floor section and site title source linkage",async()=>{
  const keys=sessionPair();
  const jpeg="/9j/"+("A".repeat(120));
  const img=(label,i)=>{
    const result={mime:"image/jpeg",base64:jpeg,label,width:1200,height:1200,
      contentBounds:[i*100,0,i*100+99,99],
      drawingBounds:[i*1000,0,i*1000+999,999]};
    if(i>0)Object.assign(result,{
      viewTitle:i===1?"BODRUM KAT PLANI":"A-A KESITI",
      viewKind:i===1?"basement":"section",viewSourceId:i+31,candidateCrop:true});
    return result;
  };
  let sent=null;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Tüm paftaları ve kotları denetle",analysisScope:"sanitary",
      cad:{schema:"musacad-cad-json/v1",items:[
        {sourceId:32,centerX:1500,centerY:200,type:"TEXT",text:"KOT: -3.20"}]},
      visualEvidence:{
        schema:"musacad-visual-evidence/v1",viewSchema:"musacad-visual-views/v1",
        viewBatch:1,viewBatchCount:1,totalCandidateViews:2,firstView:1,lastView:2,
        candidateCropsNotVerifiedViewFrames:true,rawDrawingIncluded:false,
        images:[img("full-sheet-overview",0),img("view-focus-1",1),img("view-focus-2",2)],
        complete:true
      }
    })
  });
  const res=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"test",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(_url,init)=>{
      sent=JSON.parse(init.body);
      return new Response(JSON.stringify({
        choices:[{message:{content:"Kesit ile kot karşılaştırması için ortak datum teyidi gerekir."}}]
      }),{status:200,headers:{"content-type":"application/json"}});
    }
  });
  assert.equal(res.status,200);
  const result=await res.json();
  assert.equal(result.status,"ok");
  assert.equal(result.visualRegionCount,3);
  assert.equal(result.visualCoverageComplete,true);
  assert.match(sent.messages[0].content,/candidate close-ups around labels/);
  assert.match(sent.messages[1].content[0].text,/FOCUSED VIEW BATCH: 1/);
  assert.match(sent.messages[1].content[0].text,/BODRUM KAT PLANI/);
  assert.match(sent.messages[1].content[0].text,/"viewSourceId":32/);
  assert.equal(sent.messages[1].content[3].type,"image_url");
});

test("view-focus rejects invented view extents or skipped view references",async()=>{
  const keys=sessionPair();
  const jpeg="/9j/"+("A".repeat(120));
  const image=(label,i)=>({mime:"image/jpeg",base64:jpeg,label,
    width:1200,height:1200,contentBounds:[0,0,100,100],
    drawingBounds:[0,0,100,100],
    ...(i>0?{viewTitle:"ZEMIN KAT",viewKind:"ground",viewSourceId:i,
      candidateCrop:true}:{})});
  for(const entry of [
    {title:"missing crop-approximation warning",warn:false,
      images:[image("full-sheet-overview",0),image("view-focus-1",1)]},
    {title:"wrong view label",warn:true,
      images:[image("full-sheet-overview",0),image("view-focus-2",1)]},
    {title:"false complete",warn:true,
      images:[image("full-sheet-overview",0),image("view-focus-1",1)]}
  ]){
    let called=false;
    const request=new Request("https://ai.musacad.test/v1/analyze",{
      method:"POST",headers:{
        authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
        "content-type":"application/json"},
      body:JSON.stringify({
        prompt:"katlari kontrol et",
        cad:{schema:"musacad-cad-json/v1",items:[]},
        visualEvidence:{
          schema:"musacad-visual-evidence/v1",viewSchema:"musacad-visual-views/v1",
          viewBatch:1,viewBatchCount:1,totalCandidateViews:2,firstView:1,lastView:2,
          candidateCropsNotVerifiedViewFrames:entry.warn,
          rawDrawingIncluded:false,complete:true,images:entry.images
        }
      })
    });
    const res=await worker.fetch(request,{
      AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"test",
      MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
      __fetch:async()=>{called=true;throw Error("should never forward invalid view evidence");}
    });
    assert.equal(res.status,400,entry.title);
    assert.equal(called,false,entry.title);
  }
});


test("Gemini transient 503 retries twice and then completes visual AI request",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+600000);
  let calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({prompt:"Projeyi incele",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{
      calls++;
      if(calls<3) return new Response("",{status:503});
      return new Response(JSON.stringify({choices:[{message:{content:"Bölge analizi tamamlandı."}}]}),{
        status:200,headers:{"content-type":"application/json"}
      });
    }
  });
  const body=await result.json();
  assert.equal(calls,3);
  assert.equal(result.status,200);
  assert.equal(body.status,"ok");
  assert.equal(body.reply,"Bölge analizi tamamlandı.");
});

test("Gemini persistent 503 returns useful message after only two retries",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+600000);
  let calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({prompt:"Projeyi incele",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{
      calls++;
      return new Response(JSON.stringify({error:{message:"Service Unavailable"}}),{
        status:503,headers:{"content-type":"application/json"}
      });
    }
  });
  const body=await result.json();
  assert.equal(calls,3);
  assert.equal(result.status,503);
  assert.equal(body.status,"server_error");
  assert.equal(body.providerHttpStatus,503);
  assert.match(body.message,/HTTP 503/);
  assert.match(body.message,/yeniden deneme/);
});

test("Gemini quota 429 is never retried",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+600000);
  let calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({prompt:"Projeyi incele",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{
      calls++;
      return new Response(JSON.stringify({error:{message:"Quota exceeded"}}),{
        status:429,headers:{"content-type":"application/json"}
      });
    }
  });
  assert.equal(calls,1);
  assert.equal(result.status,429);
});


test("Gemini upstream 502 is distinguishable from the gateway 503 and safely retried",async()=>{
  const keys=sessionPair();
  const token=sessionToken(keys.privateKey,Date.now()+600000);
  let calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({prompt:"Merhaba",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{
      calls++;
      return new Response("",{status:502});
    }
  });
  const body=await result.json();
  assert.equal(calls,3);
  assert.equal(result.status,503);
  assert.equal(body.providerHttpStatus,502);
  assert.match(body.message,/HTTP 502/);
});


test("Gemini native generateContent returns verified short answer without compatible chat",async()=>{
  const keys=sessionPair();
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({prompt:"Merhaba",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  let requestBody=null;
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_MODE:"native",
    GEMINI_API_KEY:"secret",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(url,init)=>{
      assert.equal(String(url),"https://generativelanguage.googleapis.com/v1beta/models/gemini-3.8-flash:generateContent");
      assert.equal(init.headers["x-goog-api-key"],"secret");
      assert.equal(init.headers.authorization,undefined);
      requestBody=JSON.parse(init.body);
      return new Response(JSON.stringify({
        candidates:[{content:{role:"model",parts:[{text:"Merhaba! Projenizi inceleyebilirim."}]}}],
        usageMetadata:{promptTokenCount:100,candidatesTokenCount:15,totalTokenCount:115}
      }),{status:200});
    }
  });
  const body=await result.json();
  assert.equal(result.status,200);
  assert.equal(body.reply,"Merhaba! Projenizi inceleyebilirim.");
  assert.equal(body.providerApi,"gemini-native");
  assert.equal(body.usage.totalTokens,115);
  assert.equal(requestBody.contents[0].parts[0].text.includes("Merhaba"),true);
  assert.ok(requestBody.systemInstruction.parts[0].text.includes("Gandalf AI"));
});

test("Gemini native visual CAD evidence and edit tools stay proposals",async()=>{
  const keys=sessionPair();
  const jpeg="/9j/"+("A".repeat(120));
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Sıhhi tesisatı incele ve şüpheli boruyu işaretlemeyi öner",
      allowEditProposals:true,analysisScope:"sanitary",
      cad:{schema:"musacad-cad-json/v1",items:[{sourceId:7,layer:"SIHHI",type:"LINE"}]},
      visualEvidence:{schema:"musacad-visual-evidence/v1",rawDrawingIncluded:false,
        complete:false,images:[{mime:"image/jpeg",base64:jpeg,label:"full-sheet-overview",
          width:800,height:800,contentBounds:[0,0,100,100],drawingBounds:[1000,2000,1100,2100]}]}
    })
  });
  let sent=null;
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_MODE:"native",
    GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(url,init)=>{
      sent=JSON.parse(init.body);
      return new Response(JSON.stringify({
        candidates:[{content:{role:"model",parts:[
          {text:"SIHHI layer boru çapı doğrulanmalı."},
          {functionCall:{name:"cad_highlight_entities",args:{sourceIds:[7],reason:"Çap teyidi"}}}
        ]}}],
        usageMetadata:{promptTokenCount:200,candidatesTokenCount:30,totalTokenCount:230}
      }),{status:200});
    }
  });
  const body=await result.json();
  assert.equal(result.status,200);
  assert.equal(body.status,"ok");
  assert.equal(body.analysisScope,"sanitary");
  assert.equal(body.visualRegionCount,1);
  assert.match(body.reply,/boru çapı/);
  assert.equal(body.actions.length,1);
  assert.equal(body.actions[0].name,"cad_highlight_entities");
  assert.equal(body.actions[0].arguments.sourceIds[0],7);
  assert.equal(body.actions[0].reason,"Çap teyidi");
  assert.equal(sent.contents[0].parts[1].inlineData.mimeType,"image/jpeg");
  assert.equal(sent.contents[0].parts[1].inlineData.data,jpeg);
  assert.equal(sent.tools[0].functionDeclarations.some(t=>t.name==="cad_highlight_entities"),true);
  assert.equal(sent.systemInstruction.parts[0].text.includes("primary discipline scope: sanitary"),true);
});

test("Gemini native 429 is not retried and retains clear quota error",async()=>{
  const keys=sessionPair();
  let calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",
    headers:{authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({prompt:"Merhaba",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const result=await worker.fetch(request,{
    AI_PROVIDER:"gemini",GEMINI_API_MODE:"native",
    GEMINI_API_KEY:"dummy",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{
      calls++;
      return new Response(JSON.stringify({error:{code:429,message:"Quota"}}),{status:429});
    }
  });
  assert.equal(result.status,429);
  assert.equal(calls,1);
});


test("self-hosted Qwen accepts signed developer session and consensual CAD images",async()=>{
  const keys=sessionPair();
  const token=developerSessionToken(keys.privateKey,Date.now()+600000);
  const img="/9j/"+("A".repeat(120));
  let sent=null, calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{authorization:"Bearer "+token,"content-type":"application/json"},
    body:JSON.stringify({
      prompt:"Mekanik çizimi ve pis su hattını incele",
      analysisScope:"wastewater",allowWeb:true,allowEditProposals:true,
      cad:{schema:"musacad-cad-json/v1",items:[{sourceId:77,type:"LINE",layer:"PIS_SU"}]},
      visualEvidence:{
        schema:"musacad-visual-evidence/v1",rawDrawingIncluded:false,
        complete:false,images:[{mime:"image/jpeg",base64:img,label:"overview",
          width:800,height:800,contentBounds:[0,0,100,100],drawingBounds:[0,0,100,100]}]
      }
    })
  });
  const response=await worker.fetch(request,{
    AI_PROVIDER:"selfhosted",
    SELFHOSTED_AI_ENDPOINT:"https://private-ai.example.com/v1/chat/completions",
    SELFHOSTED_AI_API_KEY:"server-only-secret",
    SELFHOSTED_MODEL:"qwen3.5:4b",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async(url,options)=>{
      calls++;
      assert.equal(String(url),"https://private-ai.example.com/v1/chat/completions");
      assert.equal(options.headers.authorization,"Bearer server-only-secret");
      sent=JSON.parse(options.body);
      return new Response(JSON.stringify({
        choices:[{message:{
          content:"Görülen pis su hattı adaydır; çap teyidi gerekir.",
          tool_calls:[{function:{name:"cad_highlight_entities",
            arguments:JSON.stringify({sourceIds:[77],reason:"Yerinde kontrol"})}}]
        }}],usage:{prompt_tokens:300,completion_tokens:80,total_tokens:380}
      }),{status:200,headers:{"content-type":"application/json"}});
    }
  });
  const payload=await response.json();
  assert.equal(calls,1);
  assert.equal(response.status,200);
  assert.equal(payload.status,"ok");
  assert.equal(payload.provider,"selfhosted");
  assert.equal(payload.model,"qwen3.5:4b");
  assert.equal(payload.providerApi,"selfhosted-chat-compatible");
  assert.equal(payload.accessMode,"developer");
  assert.equal(payload.analysisScope,"wastewater");
  assert.equal(payload.visualRegionCount,1);
  assert.match(payload.reply,/pis su hattı/);
  assert.equal(payload.actions.length,1);
  assert.equal(payload.actions[0].name,"cad_highlight_entities");
  assert.equal(payload.usage.totalTokens,380);
  assert.equal(sent.model,"qwen3.5:4b");
  assert.equal(sent.stream,false);
  assert.equal(sent.messages[0].role,"system");
  assert.match(sent.messages[0].content,/NO built-in live web search/);
  assert.match(sent.messages[1].content[0].text,/MUSACAD CAD-JSON/);
  assert.equal(sent.messages[1].content[1].type,"image_url");
  assert.equal(sent.messages[1].content[1].image_url.url,
    "data:image/jpeg;base64,"+img);
  assert.ok(sent.tools.some(t=>t.function.name==="cad_highlight_entities"));
  assert.equal(sent.tool_choice,undefined);
  assert.equal(payload.webUsed,false);
  assert.equal(JSON.stringify(payload).includes("server-only-secret"),false);
});

test("self-hosted provider refuses unencrypted, private and malformed endpoints",async()=>{
  const keys=sessionPair();
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{
      authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({prompt:"Merhaba",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  for(const endpoint of ["http://public.example.com/v1/chat/completions",
    "https://localhost/v1/chat/completions",
    "https://127.0.0.1/v1/chat/completions",
    "https://192.168.1.5/v1/chat/completions",
    "https://private.example.com/v1/other",
    "https://user:pass@private.example.com/v1/chat/completions",
    "https://private.example.com/v1/chat/completions?token=leak"]){
    let called=false;
    const response=await worker.fetch(request,{
      AI_PROVIDER:"selfhosted",SELFHOSTED_AI_ENDPOINT:endpoint,
      SELFHOSTED_AI_API_KEY:"secret",SELFHOSTED_MODEL:"qwen3.5:4b",
      MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
      __fetch:async()=>{called=true;throw Error("must never contact malformed endpoint");}
    });
    assert.equal(response.status,503,endpoint);
    assert.equal(called,false,endpoint);
  }
});

test("self-hosted Qwen 429 stays local and does not fall back to Gemini",async()=>{
  const keys=sessionPair();let calls=0;
  const request=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{
      authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({prompt:"Projeyi incele",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const response=await worker.fetch(request,{
    AI_PROVIDER:"selfhosted",
    SELFHOSTED_AI_ENDPOINT:"https://private.example.com/v1/chat/completions",
    SELFHOSTED_AI_API_KEY:"server-only-secret",SELFHOSTED_MODEL:"qwen3.5:4b",
    GEMINI_API_KEY:"not-used",GEMINI_MODEL:"gemini-3.8-flash",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>{
      calls++;
      return new Response(JSON.stringify({error:{message:"private error token=secret"}}),{status:429});
    }
  });
  const body=await response.json();
  assert.equal(calls,1);
  assert.equal(response.status,429);
  assert.equal(body.status,"quota_exhausted");
  assert.match(body.message,/kapasite/);
  assert.equal(JSON.stringify(body).includes("private error"),false);
});

test("self-hosted Qwen empty answer is an error, never fabricated success",async()=>{
  const keys=sessionPair();
  const req=new Request("https://ai.musacad.test/v1/analyze",{
    method:"POST",headers:{
      authorization:"Bearer "+sessionToken(keys.privateKey,Date.now()+600000),
      "content-type":"application/json"},
    body:JSON.stringify({prompt:"Merhaba",cad:{schema:"musacad-cad-json/v1",items:[]}})
  });
  const res=await worker.fetch(req,{
    AI_PROVIDER:"selfhosted",
    SELFHOSTED_AI_ENDPOINT:"https://private.example.com/v1/chat/completions",
    SELFHOSTED_AI_API_KEY:"secret",SELFHOSTED_MODEL:"qwen3.5:4b",
    MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:keys.publicPem,
    __fetch:async()=>new Response(JSON.stringify({
      choices:[{message:{content:""}}]
    }),{status:200})
  });
  assert.equal(res.status,502);
  assert.equal((await res.json()).status,"server_error");
});
