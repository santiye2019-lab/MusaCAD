/**
 * MusaCAD opt-in diagnostics to a verified PRIVATE GitHub repository.
 * The mobile client never receives a GitHub token or repository write permission.
 * Do not accept drawing bytes, names, paths, text or screenshots.
 */
const MAX_REPORT_BODY = 8192;
const MAX_DWG_BYTES = 512 * 1024 * 1024;
const EVENTS = new Set(["open_success", "open_failure", "open_partial", "open_3d"]);
const FORMATS = new Set(["DXF","UNKNOWN","AC1009","AC1012","AC1014","AC1015","AC1018","AC1021","AC1024","AC1027","AC1032"]);
const ERRORS = new Set(["none", "io", "memory", "runtime", "unknown"]);
const EXACT_FIELDS = ["schema", "id", "createdAtMs", "appVersion", "format",
  "event", "fileBytes", "entityCount", "trace", "errorKind"];

function respond(json, status = 200) {
  return new Response(JSON.stringify(json), { status, headers: {
    "content-type": "application/json; charset=utf-8",
    "cache-control": "no-store", "x-content-type-options": "nosniff"
  } });
}

async function boundedBody(request) {
  if (!request.body) return null;
  const reader = request.body.getReader();
  const chunks = []; let total = 0;
  try {
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      total += value.byteLength;
      if (total > MAX_REPORT_BODY) throw new Error("oversize");
      chunks.push(value);
    }
  } catch (_) {
    try { await reader.cancel(); } catch (_) {}
    return null;
  } finally {
    reader.releaseLock();
  }
  const raw = new Uint8Array(total); let offset = 0;
  for (const chunk of chunks) { raw.set(chunk, offset); offset += chunk.byteLength; }
  try { return JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(raw)); }
  catch (_) { return null; }
}

function validReport(report) {
  if (!report || typeof report !== "object" || Array.isArray(report)) return false;
  if (Object.keys(report).sort().join("|") !== EXACT_FIELDS.slice().sort().join("|")) return false;
  const intIn = (x, min, max) => Number.isSafeInteger(x) && x >= min && x <= max;
  return report.schema === "musacad-diagnostic-v1" &&
    typeof report.id === "string" &&
    /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i.test(report.id) &&
    intIn(report.createdAtMs, Date.now() - 90*86400000, Date.now() + 5*60000) &&
    typeof report.appVersion === "string" && /^[A-Za-z0-9._-]{1,40}$/.test(report.appVersion) &&
    FORMATS.has(report.format) && EVENTS.has(report.event) &&
    intIn(report.fileBytes, 0, MAX_DWG_BYTES) &&
    intIn(report.entityCount, 0, 10000000) &&
    typeof report.trace === "string" && report.trace.length <= 420 &&
    /^[A-Za-z0-9_.= \-]*$/.test(report.trace) &&
    ERRORS.has(report.errorKind) &&
    (report.event === "open_success" || report.event === "open_3d" ?
      report.errorKind === "none" : true);
}

async function deviceTag(deviceId) {
  const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(deviceId));
  return Array.from(new Uint8Array(digest)).slice(0, 12)
    .map(n => n.toString(16).padStart(2, "0")).join("");
}

function toBase64(str) {
  const bytes = new TextEncoder().encode(str);
  let bin = "";
  for (let i=0; i<bytes.length; i++) bin += String.fromCharCode(bytes[i]);
  return btoa(bin);
}

export async function handleDiagnostics(request, env, verifySession, fetchImpl = fetch) {
  if (request.method !== "POST") return respond({status:"method_not_allowed"},405);
  if (!env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM || !env.MUSACAD_REPORTS_GITHUB_TOKEN ||
      !env.MUSACAD_REPORTS_GITHUB_REPO)
    return respond({status:"not_configured"},503);
  const repo = String(env.MUSACAD_REPORTS_GITHUB_REPO);
  if (!/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(repo))
    return respond({status:"invalid_private_repo_config"},503);
  const session = await verifySession(request.headers.get("authorization"),
    env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM);
  if (!session.ok || typeof session.deviceId !== "string")
    return respond({status:"unauthorized"},401);

  const payload = await boundedBody(request);
  if (!validReport(payload)) return respond({status:"invalid_report"},400);
  const tag = await deviceTag(session.deviceId);
  const date = new Date(payload.createdAtMs).toISOString().slice(0,10);
  const path = `Raporlar/${date}/${tag}/${payload.id}.json`;
  const base = `https://api.github.com/repos/${repo}`;
  const headers = { authorization:`Bearer ${env.MUSACAD_REPORTS_GITHUB_TOKEN}`,
    accept:"application/vnd.github+json", "x-github-api-version":"2022-11-28",
    "user-agent":"MusaCAD-Diagnostic-Bridge/1", "content-type":"application/json" };
  try {
    // Do not upload a single byte until GitHub confirms the destination is PRIVATE.
    const check = await fetchImpl(base, {method:"GET",headers});
    if (!check.ok) return respond({status:"private_repo_unavailable"},503);
    const info = await check.json();
    if (info.private !== true || info.full_name !== repo)
      return respond({status:"private_repo_required"},503);

    const body = toBase64(JSON.stringify(payload) + "\n");
    const result = await fetchImpl(`${base}/contents/${path}`, {method:"PUT",
      headers, body:JSON.stringify({message:`diag: automatic sanitized report ${payload.id}`,
        content:body})});
    if (!result.ok) return respond({status:"github_write_failed"},502);
    return respond({status:"stored",id:payload.id});
  } catch (_) {
    return respond({status:"upload_unavailable"},503);
  }
}
