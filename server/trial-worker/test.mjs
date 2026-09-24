import test from "node:test";
import assert from "node:assert/strict";
import { generateKeyPairSync, webcrypto } from "node:crypto";
import worker from "./src/index.js";

if (!globalThis.crypto) globalThis.crypto = webcrypto;

const TRIAL_MS = 24 * 60 * 60 * 1000;
const PACKAGE = "com.musa.cad";
const DEVICE = "MC-12345678-90ABCDEF-12345678";

class FakeD1 {
  constructor(){ this.rows = new Map(); }

  prepare(sql){
    const db=this;
    return {
      bind(...args){
        return {
          async run(){
            if(sql.includes("INSERT OR IGNORE INTO trials")){
              const [deviceId,packageName,startedAt,expiresAt]=args;
              if(!db.rows.has(deviceId)){
                db.rows.set(deviceId,{
                  package_name:packageName,
                  started_at_ms:startedAt,
                  expires_at_ms:expiresAt
                });
              }
            }
            return {success:true};
          },
          async first(){
            if(sql.includes("SELECT package_name,started_at_ms,expires_at_ms FROM trials")){
              const [deviceId]=args;
              return db.rows.get(deviceId) ?? null;
            }
            return null;
          }
        };
      }
    };
  }
}

function privateKeyPem(){
  const {privateKey}=generateKeyPairSync("rsa",{modulusLength:2048});
  return privateKey.export({type:"pkcs8",format:"pem"}).toString();
}

async function start(env,deviceId=DEVICE){
  const request=new Request("https://trial.musacad.test/v1/trial/start",{
    method:"POST",
    headers:{"content-type":"application/json"},
    body:JSON.stringify({
      deviceId,
      packageName:PACKAGE,
      versionName:"1.2.0",
      versionCode:15
    })
  });
  const response=await worker.fetch(request,env);
  return {response,body:await response.json()};
}

test("reinstall does not reset the one-day trial for the same device",async()=>{
  const originalNow=Date.now;
  const t0=1_800_000_000_000;
  const env={DB:new FakeD1(),MUSACAD_TRIAL_PRIVATE_KEY_PEM:privateKeyPem(),MUSACAD_PACKAGE_NAME:PACKAGE};

  try{
    Date.now=()=>t0;
    const first=await start(env);
    assert.equal(first.response.status,200);
    assert.equal(first.body.status,"active");
    assert.equal(first.body.expiresAtMs,t0+TRIAL_MS);
    assert.match(first.body.token,/^MT1\./);

    // Simulate uninstall/reinstall: local app state is gone, but the server keeps the same device row.
    Date.now=()=>t0+(60*60*1000);
    const reinstallDuringTrial=await start(env);
    assert.equal(reinstallDuringTrial.response.status,200);
    assert.equal(reinstallDuringTrial.body.status,"active");
    assert.equal(reinstallDuringTrial.body.expiresAtMs,t0+TRIAL_MS,
      "reinstall must keep the original expiry instead of granting a fresh 24 hours");

    Date.now=()=>t0+TRIAL_MS+1;
    const reinstallAfterExpiry=await start(env);
    assert.equal(reinstallAfterExpiry.response.status,409);
    assert.equal(reinstallAfterExpiry.body.status,"used",
      "same device must not receive a second trial after reinstall");

    // A genuinely different device still gets its own first trial.
    const other=await start(env,"MC-ABCDEF12-34567890-ABCDEF12");
    assert.equal(other.response.status,200);
    assert.equal(other.body.status,"active");
  }finally{
    Date.now=originalNow;
  }
});
