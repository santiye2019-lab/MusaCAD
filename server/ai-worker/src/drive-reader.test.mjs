import test from "node:test";
import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readAuthorizedDriveDrawing } from "./drive-reader.js";

test("Drive reader validates bytes against SHA-256",async()=>{
  const bytes=new TextEncoder().encode("test drawing");
  const md5=createHash("md5").update(bytes).digest("hex");
  const sha256=createHash("sha256").update(bytes).digest("hex");
  const fileId="abcDEF0123456789";
  const fetcher=async(url,options)=>{
    assert.equal(options.headers.Authorization,"Bearer private-token");
    if(url.includes("fields="))
      return Response.json({id:fileId,size:String(bytes.length),md5Checksum:md5,trashed:false});
    return new Response(bytes,{status:200,headers:{"content-length":String(bytes.length)}});
  };
  const actual=await readAuthorizedDriveDrawing({
    fileId,accessToken:"private-token",expectedSize:bytes.length,
    expectedMd5:md5,expectedSha256:sha256,fetcher
  });
  assert.deepEqual(actual,bytes);
  await assert.rejects(()=>readAuthorizedDriveDrawing({
    fileId,accessToken:"private-token",expectedSize:bytes.length,
    expectedMd5:md5,expectedSha256:"0".repeat(64),fetcher
  }),/drive_content_checksum_mismatch/);
});

test("Drive reader rejects untrusted file identifiers",async()=>{
  await assert.rejects(()=>readAuthorizedDriveDrawing({
    fileId:"https://evil.example/x",accessToken:"secret",expectedSize:10,
    expectedMd5:"a".repeat(32),expectedSha256:"b".repeat(64)
  }),/invalid_drive_file_id/);
});

test("Drive reader rejects mismatched metadata before downloading",async()=>{
  const fileId="abcDEF0123456789";
  const bytes=new TextEncoder().encode("test drawing");
  const md5=createHash("md5").update(bytes).digest("hex");
  const sha256=createHash("sha256").update(bytes).digest("hex");
  let requests=0;
  const fetcher=async()=>{requests++;return Response.json({id:fileId,size:String(bytes.length+1),md5Checksum:md5,trashed:false});};
  await assert.rejects(readAuthorizedDriveDrawing({fileId,accessToken:"private-token",expectedSize:bytes.length,expectedMd5:md5,expectedSha256:sha256,fetcher}),/drive_metadata_mismatch/);
  assert.equal(requests,1);
});

test("Drive reader rejects oversized streamed response",async()=>{
  const fileId="abcDEF0123456789";
  const bytes=new TextEncoder().encode("test drawing");
  const md5=createHash("md5").update(bytes).digest("hex");
  const sha256=createHash("sha256").update(bytes).digest("hex");
  const fetcher=async(url)=>url.includes("fields=")
    ? Response.json({id:fileId,size:String(bytes.length),md5Checksum:md5,trashed:false})
    : new Response(new Uint8Array(bytes.length+1),{status:200});
  await assert.rejects(readAuthorizedDriveDrawing({fileId,accessToken:"private-token",expectedSize:bytes.length,expectedMd5:md5,expectedSha256:sha256,fetcher}),/drive_download_too_large/);
});
