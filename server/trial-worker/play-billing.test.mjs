import test from "node:test";
import assert from "node:assert/strict";
import { generateKeyPairSync, webcrypto } from "node:crypto";
import worker from "./src/index.js";

if (!globalThis.crypto) globalThis.crypto = webcrypto;

const PACKAGE="com.musa.cad";
const PRODUCT="musacad_pro";
const DEVICE="MC-12345678-90ABCDEF-12345678";
const OTHER_DEVICE="MC-ABCDEF12-34567890-ABCDEF12";
const PURCHASE_TOKEN="purchase-token-1234567890";

class FakeD1 {
  constructor(){ this.playRows=new Map(); }
  prepare(sql){
    const db=this;
    return {
      bind(...args){
        return {
          async first(){
            if(sql.includes("SELECT device_id,package_name,product_id FROM play_purchases")){
              const [tokenHash]=args;
              return db.playRows.get(tokenHash) ?? null;
            }
            return null;
          },
          async run(){
            if(sql.includes("INSERT OR IGNORE INTO play_purchases")){
              const [tokenHash,deviceId,packageName,productId,orderId,verifiedAt]=args;
              if(!db.playRows.has(tokenHash)){
                db.playRows.set(tokenHash,{
                  device_id:deviceId,
                  package_name:packageName,
                  product_id:productId,
                  order_id:orderId,
                  verified_at_ms:verifiedAt,
                  acknowledged_at_ms:null
                });
              }
            }else if(sql.includes("UPDATE play_purchases SET verified_at_ms")){
              const [verifiedAt,acknowledgedAt,tokenHash]=args;
              const row=db.playRows.get(tokenHash);
              if(row){
                row.verified_at_ms=verifiedAt;
                row.acknowledged_at_ms=acknowledgedAt;
              }
            }
            return {success:true};
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

function env(db,fetcher){
  return {
    DB:db,
    MUSACAD_PACKAGE_NAME:PACKAGE,
    MUSACAD_PLAY_PRODUCT_ID:PRODUCT,
    MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL:"musacad-play@test-project.iam.gserviceaccount.com",
    MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM:privateKeyPem(),
    __fetch:fetcher
  };
}

async function verify(workerEnv,deviceId=DEVICE,token=PURCHASE_TOKEN){
  const request=new Request("https://license.musacad.test/v1/play/verify",{
    method:"POST",
    headers:{"content-type":"application/json"},
    body:JSON.stringify({
      deviceId,
      packageName:PACKAGE,
      productId:PRODUCT,
      purchaseToken:token,
      versionName:"1.2.0",
      versionCode:15
    })
  });
  const response=await worker.fetch(request,workerEnv);
  return {response,body:await response.json()};
}

function googleMock({state="PURCHASED",accountId=DEVICE,acknowledged=false,productId=PRODUCT}={}){
  const calls=[];
  const fetcher=async(url,options={})=>{
    const value=String(url);
    calls.push({url:value,method:options.method||"GET"});
    if(value==="https://oauth2.googleapis.com/token"){
      assert.equal(options.method,"POST");
      assert.match(String(options.body),/grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer/);
      return new Response(JSON.stringify({access_token:"google-access-token",expires_in:3600}),{
        status:200,headers:{"content-type":"application/json"}
      });
    }
    if(value.includes("/purchases/productsv2/tokens/")){
      return new Response(JSON.stringify({
        purchaseStateContext:{purchaseState:state},
        productLineItem:[{productId}],
        obfuscatedExternalAccountId:accountId,
        orderId:"GPA.1234-5678-9012-34567",
        acknowledgementState:acknowledged
          ?"ACKNOWLEDGEMENT_STATE_ACKNOWLEDGED"
          :"ACKNOWLEDGEMENT_STATE_PENDING"
      }),{status:200,headers:{"content-type":"application/json"}});
    }
    if(value.endsWith(":acknowledge")){
      assert.equal(options.method,"POST");
      return new Response(null,{status:204});
    }
    throw new Error("Unexpected URL "+value);
  };
  return {fetcher,calls};
}

test("verified purchased product is server-bound and acknowledged before entitlement",async()=>{
  const db=new FakeD1();
  const mock=googleMock();
  const result=await verify(env(db,mock.fetcher));
  assert.equal(result.response.status,200);
  assert.equal(result.body.status,"active");
  assert.equal(result.body.productId,PRODUCT);
  assert.equal(result.body.acknowledged,true);
  assert.equal(db.playRows.size,1);
  assert.equal([...db.playRows.values()][0].device_id,DEVICE);
  assert.equal(mock.calls.filter(c=>c.url.endsWith(":acknowledge")).length,1);
});

test("pending purchase never grants MusaCAD Pro",async()=>{
  const db=new FakeD1();
  const mock=googleMock({state:"PENDING"});
  const result=await verify(env(db,mock.fetcher));
  assert.equal(result.response.status,202);
  assert.equal(result.body.status,"pending");
  assert.equal(db.playRows.size,0);
  assert.equal(mock.calls.filter(c=>c.url.endsWith(":acknowledge")).length,0);
});

test("purchase with another MusaCAD device binding is denied",async()=>{
  const db=new FakeD1();
  const mock=googleMock({accountId:OTHER_DEVICE});
  const result=await verify(env(db,mock.fetcher));
  assert.equal(result.response.status,409);
  assert.equal(result.body.status,"denied");
  assert.equal(db.playRows.size,0);
});

test("same purchase token cannot be rebound to another device",async()=>{
  const db=new FakeD1();
  const firstMock=googleMock({accountId:DEVICE,acknowledged:true});
  const first=await verify(env(db,firstMock.fetcher),DEVICE);
  assert.equal(first.body.status,"active");

  const secondMock=googleMock({accountId:OTHER_DEVICE,acknowledged:true});
  const second=await verify(env(db,secondMock.fetcher),OTHER_DEVICE);
  assert.equal(second.response.status,409);
  assert.equal(second.body.status,"denied");
  assert.equal(db.playRows.size,1);
  assert.equal([...db.playRows.values()][0].device_id,DEVICE);
});

test("cancelled or wrong product purchases are denied",async()=>{
  const cancelled=googleMock({state:"CANCELLED"});
  const cancelledResult=await verify(env(new FakeD1(),cancelled.fetcher));
  assert.equal(cancelledResult.response.status,403);
  assert.equal(cancelledResult.body.status,"denied");

  const wrong=googleMock({productId:"other_product"});
  const wrongResult=await verify(env(new FakeD1(),wrong.fetcher));
  assert.equal(wrongResult.response.status,403);
  assert.equal(wrongResult.body.status,"denied");
});
