import test from "node:test";
import assert from "node:assert/strict";
import { startDriveUpload, queryDriveUpload, uploadDriveChunks, DriveUploadError } from "./drive-upload.js";
const response = (status, headers = {}, json = {}) => ({
  status, headers: { get: key => headers[key] || null }, json: async () => json
});
test("start session validates Google upload host", async () => {
  const url = await startDriveUpload({ token: "test", name: "drawing.dwg", size: 3,
    fetcher: async () => response(200, { Location: "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&upload_id=1" }) });
  assert.match(url, /upload_id=1/);
  await assert.rejects(startDriveUpload({ token: "test", name: "a.dwg", size: 3,
    fetcher: async () => response(200, { Location: "https://evil.example/steal" }) }), /invalid_session/);
});
test("resume offset is taken from server", async () => {
  const r = await queryDriveUpload({ token: "test", size: 10,
    session: "https://www.googleapis.com/upload/drive/v3/files?upload_id=1",
    fetcher: async () => response(308, { Range: "bytes=0-4" }) });
  assert.equal(r.offset, 5);
});
test("progress only reaches 100 after verified file metadata", async () => {
  const progress = [];
  const bytes = new Uint8Array(300000);
  let count = 0;
  const file = await uploadDriveChunks({ token: "test", bytes,
    session: "https://www.googleapis.com/upload/drive/v3/files?upload_id=1",
    chunkSize: 262144, onProgress: p => progress.push(p),
    fetcher: async () => ++count === 1
      ? response(308, { Range: "bytes=0-262143" })
      : response(200, {}, { id: "file-id", size: "300000" }) });
  assert.equal(file.id, "file-id");
  assert.deepEqual(progress, [87, 100]);
});
test("does not claim success on rejected upload", async () => {
  const progress = [];
  await assert.rejects(uploadDriveChunks({ token: "test", bytes: new Uint8Array(300000),
    session: "https://www.googleapis.com/upload/drive/v3/files?upload_id=1",
    onProgress: p => progress.push(p), fetcher: async () => response(401) }), DriveUploadError);
  assert.deepEqual(progress, []);
});

test("rejects fake Google path even on legitimate host", async () => {
  await assert.rejects(startDriveUpload({ token: "test", name: "a.dwg", size: 3,
    fetcher: async () => response(200, { Location: "https://www.googleapis.com/upload/drive/v3/files-evil" }) }),
    /invalid_session/);
});
test("rejects completion without verified size", async () => {
  await assert.rejects(uploadDriveChunks({ token: "test", bytes: new Uint8Array(300000),
    session: "https://www.googleapis.com/upload/drive/v3/files?upload_id=1",
    fetcher: async () => response(200, {}, { id: "file-id" }) }), /unverified_completion/);
});
test("rejects completed resume without matching size", async () => {
  await assert.rejects(queryDriveUpload({ token: "test", size: 10,
    session: "https://www.googleapis.com/upload/drive/v3/files?upload_id=1",
    fetcher: async () => response(200, {}, { id: "file-id", size: "9" }) }), /unverified_completion/);
});
