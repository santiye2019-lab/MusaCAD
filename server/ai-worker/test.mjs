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
  assert.match(body.message,/süresi içinde|gecikti/);
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
  assert.equal(body.message.includes("ücretsiz kullanım kotası"),true);
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
