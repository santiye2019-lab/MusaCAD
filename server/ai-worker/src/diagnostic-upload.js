// Bounded, developer-session-only diagnostics transport.
// Only a server-side GitHub token can write to the public Raporlar directory.
// Never store filenames, file hashes, DWG content, coordinates or error messages.
const TARGET_REPO = "santiye2019-lab/MusaCAD";
const MAX_BYTES = 4096;
const FIELDS = [
  "schema", "reportId", "occurredAt", "appVersion", "format", "signature",
  "result", "stage", "elapsedMs", "inputBytes", "parsedEntities",
  "layerCount", "conversionWarnings", "errorCode"
];
const SIGNATURES = new Set([
  "DXF", "UNKNOWN", "AC1009", "AC1012", "AC1014", "AC1015",
  "AC1018", "AC1021", "AC1024", "AC1027", "AC1032"
]);
const STAGES = new Set([
  "COPY", "NATIVE_OPEN", "NATIVE_SCENE", "DXF_EXPORT", "DXF_PARSE",
  "VIEW_HANDOFF", "UNKNOWN"
]);
const ERRORS = new Set(["NONE", "IO", "OOM", "CANCELLED", "OTHER"]);

function validInt(value, max) {
  return typeof value === "number" && Number.isSafeInteger(value) &&
    value >= 0 && value <= max;
}

/** Return a canonical whitelist copy. Never forward untrusted JSON verbatim. */
export function validateDiagnostic(body) {
  if (!body || typeof body !== "object" || Array.isArray(body)) return null;
  if (Object.keys(body).length !== FIELDS.length ||
      Object.keys(body).some(k => !FIELDS.includes(k)) ||
      FIELDS.some(k => !Object.prototype.hasOwnProperty.call(body, k))) return null;
  if (body.schema !== "musacad-diagnostics/v1" ||
      !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(body.reportId) ||
      typeof body.occurredAt !== "string" ||
      !/^20[0-9]{2}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$/.test(body.occurredAt) ||
      !Number.isFinite(Date.parse(body.occurredAt)) ||
      typeof body.appVersion !== "string" ||
      !/^[a-zA-Z0-9._+-]{1,24}$/.test(body.appVersion) ||
      !["DWG", "DXF"].includes(body.format) ||
      !SIGNATURES.has(body.signature) ||
      (body.format === "DXF" && body.signature !== "DXF") ||
      (body.format === "DWG" && body.signature === "DXF") ||
      !["success", "partial", "failure"].includes(body.result) ||
      !STAGES.has(body.stage) || !ERRORS.has(body.errorCode) ||
      !validInt(body.elapsedMs, 86400000) ||
      !validInt(body.inputBytes, 536870912) ||
      !validInt(body.parsedEntities, 10000000) ||
      !validInt(body.layerCount, 1000000) ||
      !validInt(body.conversionWarnings, 1000000)) return null;
  if (body.result === "success" && body.errorCode !== "NONE") return null;
  return Object.fromEntries(FIELDS.map(k => [k, body[k]]));
}

function encodeUtf8Base64(value) {
  const bytes = new TextEncoder().encode(value);
  let ascii = "";
  for (const byte of bytes) ascii += String.fromCharCode(byte);
  return btoa(ascii);
}

function githubHeaders(token) {
  return {
    "authorization": "Bearer " + token,
    "accept": "application/vnd.github+json",
    "x-github-api-version": "2022-11-28",
    "content-type": "application/json",
    "user-agent": "MusaCAD-diagnostic-reporter"
  };
}

export async function handleDiagnosticUpload(request, env, session) {
  if (request.method !== "POST") return new Response(JSON.stringify({status:"method_not_allowed"}), {
    status:405, headers:{"content-type":"application/json", "allow":"POST"}
  });
  if (!session?.ok) return reply(401, "denied");
  // Public repo can only receive developer-initiated, sanitized diagnostic data.
  if (session.mode !== "developer") return reply(403, "developer_only");
  const token = String(env.MUSACAD_REPORT_GITHUB_TOKEN || "");
  if (!token) return reply(503, "not_configured");
  if (Number(request.headers.get("content-length")) > MAX_BYTES) return reply(413, "too_large");
  let textBody;
  try { textBody = await request.text(); } catch (_) { return reply(400, "invalid_json"); }
  if (new TextEncoder().encode(textBody).length > MAX_BYTES) return reply(413, "too_large");
  let raw;
  try { raw = JSON.parse(textBody); } catch (_) { return reply(400, "invalid_json"); }
  const report = validateDiagnostic(raw);
  if (!report) return reply(400, "invalid_report");

  // Stable UUID path makes network retries idempotent. No user-controlled path.
  const path = "Raporlar/" + report.occurredAt.slice(0, 10) + "/" + report.reportId + ".json";
  const url = "https://api.github.com/repos/" + TARGET_REPO + "/contents/" + path;
  const content = JSON.stringify(report, null, 2) + "\n";
  const transport = typeof env.__fetch === "function" ? env.__fetch : fetch;
  const headers = githubHeaders(token);
  try {
    const put = await transport(url, {
      method:"PUT", headers,
      body:JSON.stringify({
        message:"chore(diagnostics): store anonymous DWG opening telemetry",
        content:encodeUtf8Base64(content), branch:"main"
      })
    });
    if (put.status === 201) return reply(201, "stored", {path});
    if (put.status === 422) {
      // GitHub returns 422 if this exact path already exists. Verify identical
      // content before acknowledging a retry; never overwrite an existing file.
      const existing = await transport(url, {method:"GET", headers});
      if (existing.status === 200) {
        const data = await existing.json();
        if (typeof data.content === "string" &&
            data.content.replace(/\s/g, "") === encodeUtf8Base64(content))
          return reply(200, "stored", {path});
      }
      return reply(409, "conflict");
    }
    if (put.status === 401 || put.status === 403) return reply(503, "github_auth_or_quota");
    return reply(502, "github_unavailable");
  } catch (_) {
    return reply(502, "github_unavailable");
  }
}

function reply(status, state, extra={}) {
  return new Response(JSON.stringify({status:state, ...extra}), {
    status, headers: {
      "content-type":"application/json; charset=utf-8",
      "cache-control":"no-store",
      "x-content-type-options":"nosniff"
    }
  });
}
