import test from "node:test";
import assert from "node:assert/strict";
import {createHash} from "node:crypto";
import {resolveDriveGrant} from "./drive-grant.js";
import {readAuthorizedDriveDrawing} from "./drive-reader.js";
import {extractCadEvidence} from "./cad-extract.js";

const drawing="0\nSECTION\n2\nENTITIES\n0\nLINE\n8\nMEKANIK\n10\n0\n20\n0\n11\n10\n21\n0\n0\nENDSEC\n0\nEOF\n";
const bytes=new TextEncoder().encode(drawing);
const md5=createHash("md5").update(bytes).digest("hex");
const sha256=createHash("sha256").update(bytes).digest("hex");
const identity={deviceId:"device-1",projectId:"project_1",fileId:"abcDEF0123456789"};

test("authorized Drive drawing yields bounded CAD evidence without retaining a raw file",async()=>{
  const broker={fetch:async request=>{
    assert.deepEqual(await request.json(),identity);
    return Response.json({...identity,accessToken:"ephemeral-test-token"});
  }};
  const token=await resolveDriveGrant(broker,identity);
  const fetcher=async(url,options)=>{
    assert.equal(options.headers.Authorization,"Bearer ephemeral-test-token");
    return url.includes("fields=")
      ? Response.json({id:identity.fileId,size:String(bytes.length),md5Checksum:md5,trashed:false})
      : new Response(bytes,{status:200,headers:{"content-length":String(bytes.length)}});
  };
  const downloaded=await readAuthorizedDriveDrawing({fileId:identity.fileId,accessToken:token,expectedSize:bytes.length,expectedMd5:md5,expectedSha256:sha256,fetcher});
  const evidence=extractCadEvidence(downloaded);
  assert.equal(evidence.format,"ASCII_DXF");
  assert.equal(evidence.totalEntities,1);
  assert.deepEqual(evidence.layers,["MEKANIK"]);
  downloaded.fill(0);
  assert.ok(downloaded.every(value=>value===0));
});

test("Drive broker denial prevents any file download",async()=>{
  let downloaded=false;
  const broker={fetch:async()=>new Response(null,{status:403})};
  await assert.rejects(async()=>{
    const token=await resolveDriveGrant(broker,identity);
    downloaded=true;
    await readAuthorizedDriveDrawing({fileId:identity.fileId,accessToken:token,expectedSize:bytes.length,expectedMd5:md5,expectedSha256:sha256});
  },/drive_access_denied/);
  assert.equal(downloaded,false);
});

test("binary DWG cannot be mislabeled as analyzed",()=>{
  const dwg=new TextEncoder().encode("AC1032 example DWG payload");
  assert.throws(()=>extractCadEvidence(dwg),/binary_dwg_decoder_required/);
});
