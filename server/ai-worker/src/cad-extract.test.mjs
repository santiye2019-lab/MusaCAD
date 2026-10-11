import test from "node:test";
import assert from "node:assert/strict";
import {extractCadEvidence} from "./cad-extract.js";
import {resolveDriveGrant} from "./drive-grant.js";
const encoder=new TextEncoder();
test("extracts basic ASCII DXF line evidence",()=>{
 const raw=["0","SECTION","2","ENTITIES","0","LINE","8","WATER","410","Ground","10","0","20","1","11","3","21","4","0","ENDSEC","0","EOF"].join("\n");
 const result=extractCadEvidence(encoder.encode(raw));
 assert.equal(result.format,"ASCII_DXF");
 assert.equal(result.entities[0].layer,"WATER");
 assert.equal(result.entities[0].g11,3);
 assert.equal(result.coverageComplete,true);
});
test("does not pretend DWG is decoded",()=>{
 assert.throws(()=>extractCadEvidence(encoder.encode("AC1032 rest of drawing")),/binary_dwg_decoder_required/);
});
test("Drive grant must match signed device and project",async()=>{
 const broker={fetch:async()=>Response.json({deviceId:"device-1",projectId:"p-1",fileId:"abcdefghijklmno",accessToken:"secret"})};
 assert.equal(await resolveDriveGrant(broker,{deviceId:"device-1",projectId:"p-1",fileId:"abcdefghijklmno"}),"secret");
 await assert.rejects(()=>resolveDriveGrant(broker,{deviceId:"device-2",projectId:"p-1",fileId:"abcdefghijklmno"}),/drive_grant_mismatch/);
});
