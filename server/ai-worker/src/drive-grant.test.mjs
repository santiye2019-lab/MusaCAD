import test from "node:test";
import assert from "node:assert/strict";
import { resolveDriveGrant } from "./drive-grant.js";

const identity={deviceId:"MC-TEST-DEVICE",projectId:"project_123",fileId:"file_1234567890"};

test("Drive broker grants only exact device/project/file binding",async()=>{
  const binding={fetch:async request=>{
    assert.equal(new URL(request.url).pathname,"/v1/resolve");
    assert.equal(request.method,"POST");
    assert.deepEqual(await request.json(),identity);
    return Response.json({...identity,accessToken:"test-token"});
  }};
  assert.equal(await resolveDriveGrant(binding,identity),"test-token");
});

test("Drive broker rejects missing binding and invalid project identity",async()=>{
  await assert.rejects(resolveDriveGrant(null,identity),/drive_broker_unconfigured/);
  await assert.rejects(resolveDriveGrant({fetch:async()=>Response.json({})},{...identity,projectId:"!"}),/invalid_project_binding/);
});

test("Drive broker rejects non-200 and mismatched authorization",async()=>{
  await assert.rejects(resolveDriveGrant({fetch:async()=>new Response(null,{status:403})},identity),/drive_access_denied/);
  for(const changed of [{deviceId:"other"},{projectId:"other"},{fileId:"other"},{accessToken:""}]){
    const binding={fetch:async()=>Response.json({...identity,accessToken:"test-token",...changed})};
    await assert.rejects(resolveDriveGrant(binding,identity),/drive_grant_mismatch/);
  }
});
