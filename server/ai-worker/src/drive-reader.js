// Authenticated server-side Drive reader primitive. Not exposed as a public route.
// Call only after checking the AI session, account binding and project ownership.
const ID=/^[A-Za-z0-9_-]{10,200}$/;
export async function readAuthorizedDriveDrawing({fileId,accessToken,expectedSize,expectedMd5,expectedSha256,fetcher=fetch,maxBytes=512*1024*1024}){
  if(!ID.test(fileId||""))throw new Error("invalid_drive_file_id");
  if(typeof accessToken!=="string"||!accessToken)throw new Error("drive_authorization_required");
  if(!Number.isSafeInteger(expectedSize)||expectedSize<1||expectedSize>maxBytes)throw new Error("invalid_expected_size");
  if(!/^[a-f0-9]{32}$/i.test(expectedMd5||""))throw new Error("invalid_expected_checksum");
  const root="https://www.googleapis.com/drive/v3/files/"+encodeURIComponent(fileId);
  const headers={Authorization:"Bearer "+accessToken};
  const meta=await fetcher(root+"?fields=id,size,md5Checksum,trashed,mimeType",{headers,redirect:"manual"});
  if(meta.status!==200)throw new Error("drive_metadata_http_"+meta.status);
  const m=await meta.json();
  if(m.id!==fileId||m.trashed||Number(m.size)!==expectedSize||
      String(m.md5Checksum||"").toLowerCase()!==expectedMd5.toLowerCase())
    throw new Error("drive_metadata_mismatch");
  const body=await fetcher(root+"?alt=media",{headers,redirect:"manual"});
  if(body.status!==200)throw new Error("drive_download_http_"+body.status);
  const declared=body.headers.get("content-length");
  if(declared!==null&&Number(declared)!==expectedSize)throw new Error("drive_download_size_mismatch");
  if(!body.body)throw new Error("drive_empty_stream");
  let total=0;
  const chunks=[];
  const reader=body.body.getReader();
  try{
    while(true){
      const {done,value}=await reader.read();
      if(done)break;
      total+=value.byteLength;
      if(total>expectedSize||total>maxBytes)throw new Error("drive_download_too_large");
      chunks.push(value);
    }
  }finally{reader.releaseLock();}
  if(total!==expectedSize)throw new Error("drive_download_incomplete");
  const bytes=new Uint8Array(total);let pos=0;
  for(const chunk of chunks){bytes.set(chunk,pos);pos+=chunk.length;}
  if(!/^[a-f0-9]{64}$/i.test(expectedSha256||""))
    throw new Error("drive_sha256_manifest_required");
  const digest=new Uint8Array(await crypto.subtle.digest("SHA-256",bytes));
  const actual=Array.from(digest,b=>b.toString(16).padStart(2,"0")).join("");
  if(actual.toLowerCase()!==expectedSha256.toLowerCase())
    throw new Error("drive_content_checksum_mismatch");
  return bytes;
}
