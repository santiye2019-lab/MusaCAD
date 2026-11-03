import test from "node:test";
import assert from "node:assert/strict";
import { handleDiagnostics } from "./src/diagnostics.js";

const ENV = {
  MUSACAD_AI_SESSION_PUBLIC_KEY_PEM:"test-only",
  MUSACAD_REPORTS_GITHUB_TOKEN:"server-secret-never-in-mobile",
  MUSACAD_REPORTS_GITHUB_REPO:"santiye2019-lab/MusaCAD-Private-Reports"
};
const authorized = async () => ({ok:true,deviceId:"MC-11111111-22222222-33333333"});
function payload() {
  return {
    schema:"musacad-diagnostic-v1",
    id:"bc15d33b-5a9c-4d3f-b88b-157932d312ee",
    createdAtMs:Date.now(),appVersion:"1.2.2",
    format:"AC1024",event:"open_success",fileBytes:4728533,
    entityCount:1234,trace:"inputMiB=4.5 objects=1234 COPY_DONEMs=123 NATIVE_OPENMs=456 sampledHeapMiB=52.1",
    errorKind:"none"
  };
}
function req(p) {
  return new Request("https://worker.example/v1/diagnostics", {
    method:"POST",headers:{authorization:"Bearer test", "content-type":"application/json"},
    body:JSON.stringify(p)
  });
}

test("stores only allowlisted report in an explicitly private GitHub repo", async () => {
  const calls=[];
  const fakeFetch=async (url,opts) => {
    calls.push({url,opts});
    if(opts.method==="GET")return Response.json({
      private:true,full_name:ENV.MUSACAD_REPORTS_GITHUB_REPO
    });
    return Response.json({content:{sha:"abc"}}, {status:201});
  };
  const record=payload();
  const response=await handleDiagnostics(req(record),ENV,authorized,fakeFetch);
  assert.equal(response.status,200);
  assert.deepEqual(await response.json(),{status:"stored",id:record.id});
  assert.equal(calls.length,2);
  assert.match(calls[1].url,/\/contents\/Raporlar\/\d{4}-\d{2}-\d{2}\/\w+\/bc15d33b.*\.json$/);
  const transmitted=JSON.parse(Buffer.from(JSON.parse(calls[1].opts.body).content,"base64").toString());
  assert.deepEqual(transmitted,record);
  assert.ok(!JSON.stringify(transmitted).includes("fileName"));
});

test("refuses a public destination before any GitHub write", async () => {
  const calls=[];
  const fakeFetch=async (url,opts)=>{
    calls.push(opts.method);
    return Response.json({private:false,full_name:ENV.MUSACAD_REPORTS_GITHUB_REPO});
  };
  const response=await handleDiagnostics(req(payload()),ENV,authorized,fakeFetch);
  assert.equal(response.status,503);
  assert.deepEqual(calls,["GET"]);
});

test("rejects extra fields with project names, screenshots or drawing data", async () => {
  const bad=payload();bad.projectName="Karagumruk.dwg";
  let called=false;
  const response=await handleDiagnostics(req(bad),ENV,authorized,async()=>{called=true;});
  assert.equal(response.status,400);assert.equal(called,false);
});

test("rejects missing signed session without connecting to GitHub", async () => {
  let called=false;
  const response=await handleDiagnostics(req(payload()),ENV,
    async()=>({ok:false}),async()=>{called=true;});
  assert.equal(response.status,401);assert.equal(called,false);
});

test("refuses uploads until the private repo and server token are configured", async () => {
  const response=await handleDiagnostics(req(payload()),
    {...ENV,MUSACAD_REPORTS_GITHUB_TOKEN:""},authorized);
  assert.equal(response.status,503);
});
