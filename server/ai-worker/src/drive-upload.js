// Google Drive resumable upload transport for a user-authorized client.
// No tokens are persisted or logged. This module is NOT wired to production.
export const DRIVE_SCOPE = "https://www.googleapis.com/auth/drive.file";
const START = "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&fields=id,name,size,md5Checksum";
const CHUNK = 256 * 1024;
export class DriveUploadError extends Error {
  constructor(code, status = 0) { super(code); this.name = "DriveUploadError"; this.code = code; this.status = status; }
}
function classify(status) {
  if (status === 401) return "authorization_expired";
  if (status === 403) return "permission_or_quota";
  if (status === 404 || status === 410) return "session_expired";
  if (status === 429) return "rate_limited";
  if (status >= 500) return "server_unavailable";
  return "upload_rejected";
}
async function checked(response, expected) {
  if (!expected.includes(response.status)) throw new DriveUploadError(classify(response.status), response.status);
  return response;
}
function validatedSession(url) {
  let u;
  try { u = new URL(url); } catch (_) { throw new DriveUploadError("invalid_session"); }
  if (u.protocol !== "https:" || u.hostname !== "www.googleapis.com" ||
      !/^\/upload\/drive\/v3\/files(?:\/|$)/.test(u.pathname))
    throw new DriveUploadError("invalid_session");
  return u.href;
}
function headers(token, extra = {}) {
  if (!token || typeof token !== "string") throw new DriveUploadError("authorization_required");
  return { Authorization: "Bearer " + token, ...extra };
}
export async function startDriveUpload({ token, name, size, parentId, fetcher = fetch }) {
  if (typeof name !== "string" || !name.trim() || name.length > 240 ||
      !Number.isSafeInteger(size) || size < 0)
    throw new DriveUploadError("invalid_file");
  const metadata = { name, mimeType: "application/octet-stream" };
  if (parentId) metadata.parents = [parentId];
  const response = await checked(await fetcher(START, {
    method: "POST", headers: headers(token, {
      "Content-Type": "application/json; charset=UTF-8",
      "X-Upload-Content-Type": "application/octet-stream",
      "X-Upload-Content-Length": String(size)
    }), body: JSON.stringify(metadata)
  }), [200, 201]);
  const session = response.headers.get("Location");
  if (!session) throw new DriveUploadError("missing_session");
  return validatedSession(session);
}
export async function queryDriveUpload({ token, session, size, fetcher = fetch }) {
  const response = await fetcher(validatedSession(session), {
    method: "PUT", headers: headers(token, { "Content-Range": "bytes */" + size })
  });
  if (response.status === 200 || response.status === 201) {
    const file = await response.json();
    if (!file || typeof file.id !== "string" || !file.id ||
        !Number.isSafeInteger(Number(file.size)) || Number(file.size) !== size)
      throw new DriveUploadError("unverified_completion");
    return { complete: true, file };
  }
  await checked(response, [308]);
  const range = response.headers.get("Range");
  if (!range) return { complete: false, offset: 0 };
  const match = /^bytes=0-(\d+)$/.exec(range);
  if (!match) throw new DriveUploadError("invalid_server_range");
  const offset = Number(match[1]) + 1;
  if (!Number.isSafeInteger(offset) || offset > size) throw new DriveUploadError("invalid_server_range");
  return { complete: false, offset };
}
export async function uploadDriveChunks({
  token, session, bytes, startOffset = 0, chunkSize = 1024 * 1024,
  fetcher = fetch, onProgress = () => {}
}) {
  if (!(bytes instanceof Uint8Array) ||
      !Number.isSafeInteger(startOffset) || startOffset < 0 || (bytes.length > 0 && startOffset >= bytes.length) ||
      !Number.isSafeInteger(chunkSize) || chunkSize < CHUNK || chunkSize % CHUNK)
    throw new DriveUploadError("invalid_chunk");
  const url = validatedSession(session);
  if (bytes.length === 0) {
    const result = await queryDriveUpload({ token, session: url, size: 0, fetcher });
    if (!result.complete) throw new DriveUploadError("unverified_completion");
    onProgress(100);
    return result.file;
  }
  let offset = startOffset;
  while (offset < bytes.length) {
    const end = Math.min(bytes.length, offset + chunkSize);
    const response = await fetcher(url, {
      method: "PUT", headers: headers(token, {
        "Content-Type": "application/octet-stream",
        "Content-Range": `bytes ${offset}-${end - 1}/${bytes.length}`
      }), body: bytes.slice(offset, end)
    });
    if (response.status === 200 || response.status === 201) {
      const file = await response.json();
      if (!file || typeof file.id !== "string" || !file.id ||
          !Number.isSafeInteger(Number(file.size)) || Number(file.size) !== bytes.length)
        throw new DriveUploadError("unverified_completion");
      onProgress(100);
      return file;
    }
    await checked(response, [308]);
    const range = response.headers.get("Range");
    const match = range && /^bytes=0-(\d+)$/.exec(range);
    const next = match ? Number(match[1]) + 1 : 0;
    if (!Number.isSafeInteger(next) || next <= offset || next > end)
      throw new DriveUploadError("unexpected_server_offset");
    offset = next;
    onProgress(Math.min(99, Math.floor(offset * 100 / bytes.length)));
  }
  throw new DriveUploadError("unverified_completion");
}
