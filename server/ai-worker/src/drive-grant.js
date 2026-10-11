// Service binding for a separately deployed, account-authorized Drive broker.
// Broker's durable OAuth vault is intentionally outside this AI worker.
// This module never accepts caller-provided OAuth tokens.
const FILE=/^[A-Za-z0-9_-]{10,200}$/;
const PROJECT=/^[A-Za-z0-9_-]{1,128}$/;
export async function resolveDriveGrant(binding,{deviceId,projectId,fileId}){
  if(!binding||typeof binding.fetch!=="function")throw new Error("drive_broker_unconfigured");
  if(!deviceId||!PROJECT.test(projectId||"")||!FILE.test(fileId||""))
    throw new Error("invalid_project_binding");
  const response=await binding.fetch(new Request("https://drive-broker.internal/v1/resolve",{
    method:"POST",headers:{"content-type":"application/json"},
    body:JSON.stringify({deviceId,projectId,fileId})
  }));
  if(response.status!==200)throw new Error("drive_access_denied");
  const grant=await response.json();
  if(grant.deviceId!==deviceId||grant.projectId!==projectId||
     grant.fileId!==fileId||typeof grant.accessToken!=="string"||!grant.accessToken)
    throw new Error("drive_grant_mismatch");
  return grant.accessToken;
}
