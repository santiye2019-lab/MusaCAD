import test from "node:test";
import assert from "node:assert/strict";
import { planProjectRetention, compareManifestRevision, MAX_OPEN_PROJECTS }
  from "./src/project-retention.js";

const A="00000000-0000-4000-8000-00000000000a";
const B="00000000-0000-4000-8000-00000000000b";
const C="00000000-0000-4000-8000-00000000000c";
const D="00000000-0000-4000-8000-00000000000d";
const E="00000000-0000-4000-8000-00000000000e";
const done=(id,completedAtMs)=>({id,completedAtMs});

test("one open project survives its own verified upload",()=>{
  const p=planProjectRetention({completedProjects:[done(A,1)],openProjectIds:[A]});
  assert.deepEqual(p.keepProjectIds,[A]);
  assert.deepEqual(p.deleteProjectIds,[]);
});

test("all simultaneously open discipline projects remain stored",()=>{
  const p=planProjectRetention({
    completedProjects:[done(A,1),done(B,2),done(C,3)],
    openProjectIds:[A,B,C]
  });
  assert.deepEqual(p.keepProjectIds,[A,B,C]);
  assert.deepEqual(p.deleteProjectIds,[]);
});

test("closing B while A and C remain open permits only B removal",()=>{
  const p=planProjectRetention({
    completedProjects:[done(A,1),done(B,2),done(C,3)],
    openProjectIds:[A,C]
  });
  assert.deepEqual(p.keepProjectIds,[A,C]);
  assert.deepEqual(p.deleteProjectIds,[B]);
});

test("no tabs open: retain latest verified project, not every old project",()=>{
  const p=planProjectRetention({
    completedProjects:[done(A,1),done(B,2),done(C,3)],
    openProjectIds:[]
  });
  assert.deepEqual(p.keepProjectIds,[C]);
  assert.deepEqual(p.deleteProjectIds,[A,B]);
});

test("opening new project never deletes last backup before upload confirms",()=>{
  const pending=planProjectRetention({
    completedProjects:[done(A,1)],openProjectIds:[D]
  });
  assert.deepEqual(pending.keepProjectIds,[A]);
  assert.deepEqual(pending.deleteProjectIds,[]);
  assert.equal(pending.needsPendingUpload,true);
  const committed=planProjectRetention({
    completedProjects:[done(A,1),done(D,4)],openProjectIds:[D]
  });
  assert.deepEqual(committed.keepProjectIds,[D]);
  assert.deepEqual(committed.deleteProjectIds,[A]);
});

test("another open project is preserved when new upload is confirmed",()=>{
  const p=planProjectRetention({
    completedProjects:[done(A,1),done(B,2),done(D,4)],
    openProjectIds:[B,D]
  });
  assert.deepEqual(p.keepProjectIds,[B,D]);
  assert.deepEqual(p.deleteProjectIds,[A]);
});

test("open but not uploaded drawing stays pending rather than fabricating completion",()=>{
  const p=planProjectRetention({
    completedProjects:[done(A,1)],openProjectIds:[A,E]
  });
  assert.deepEqual(p.keepProjectIds,[A]);
  assert.equal(p.needsPendingUpload,true);
});

test("maximum four open projects and unique UUID required",()=>{
  assert.equal(MAX_OPEN_PROJECTS,4);
  assert.throws(()=>planProjectRetention({
    completedProjects:[done(A,1)],openProjectIds:[A,B,C,D,E]
  }));
  assert.throws(()=>planProjectRetention({
    completedProjects:[done(A,1)],openProjectIds:[A,A]
  }));
  assert.throws(()=>planProjectRetention({
    completedProjects:[],openProjectIds:["somewhere/../../file"]
  }));
});

test("reject late open-tab revisions and accept idempotent retries",()=>{
  assert.equal(compareManifestRevision(8,7),"stale");
  assert.equal(compareManifestRevision(8,8),"idempotent");
  assert.equal(compareManifestRevision(8,9),"advance");
  assert.throws(()=>compareManifestRevision(8,-1));
});
