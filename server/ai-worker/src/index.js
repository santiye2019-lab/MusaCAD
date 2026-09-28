const MAX_BODY_BYTES = 1024 * 1024;
const MAX_PROMPT_CHARS = 12000;
const OPENAI_RESPONSES_URL = "https://api.openai.com/v1/responses";

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === "/health") return json({ status: "ok", service: "musacad-ai-worker" });
    if (url.pathname === "/v1/analyze") return handleAnalyze(request, env);
    return json({ status: "not_found" }, 404);
  }
};

async function handleAnalyze(request, env) {
  if (request.method !== "POST") return json({ status: "method_not_allowed" }, 405, { Allow: "POST" });
  if (!env.OPENAI_API_KEY || !env.OPENAI_MODEL || !env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM)
    return json({ status: "server_error", message: "AI worker is not fully configured" }, 503);

  const session = await verifySession(request.headers.get("authorization"), env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM);
  if (!session.ok) return json({ status: "denied", message: session.message }, 401);

  const parsed = await readJson(request);
  if (!parsed.ok) return parsed.response;
  const body = parsed.body || {};
  const prompt = String(body.prompt || "").trim();
  const cad = body.cad;
  const allowWeb = body.allowWeb === true;
  const allowEditProposals = body.allowEditProposals === true;

  if (!prompt || prompt.length > MAX_PROMPT_CHARS)
    return json({ status: "denied", message: "Prompt is empty or too large" }, 400);
  if (!cad || typeof cad !== "object" || cad.schema !== "musacad-cad-json/v1")
    return json({ status: "denied", message: "Unsupported CAD-JSON payload" }, 400);
  if (cad.cloudPolicy && cad.cloudPolicy.rawDrawingIncluded === true)
    return json({ status: "denied", message: "Raw drawing upload is not accepted by this endpoint" }, 400);

  const tools = [];
  if (allowWeb) tools.push({ type: "web_search" });
  if (allowEditProposals) tools.push(...cadProposalTools());

  const accessMode = session.mode === "developer" ? "developer" : "licensed";
  const instructions =
    "You are Gandalf AI inside MusaCAD, an engineering CAD assistant. " +
    "Current access mode: " + accessMode + ". " +
    (accessMode === "developer"
      ? "Developer mode may use the full bounded analysis and proposal surface, but drawing edits still require explicit user approval. "
      : "") +
    "Analyze the supplied bounded CAD-JSON, especially mechanical/plumbing/HVAC/fire/gas systems when present. " +
    "Separate observations from assumptions and recommendations. Never claim a drawing is code-compliant, safe, or approved merely from this data. " +
    "Call out missing information and confidence limits. " +
    "If edit tools are available, tool calls are PROPOSALS ONLY. They are not executed automatically and require explicit user approval in MusaCAD. " +
    "For cad_change_layer, cad_add_line and cad_add_text, use an exact existing layer name visible in the supplied CAD-JSON; never invent a new layer name. " +
    "Never state that a proposed edit has already been applied. " +
    "Prefer sourceId-based edits for existing entities. Use web search only when it materially helps the user's request, and identify external sources in the answer.";

  const input = [
    {
      role: "user",
      content:
        "USER REQUEST:\n" + prompt +
        "\n\nMUSACAD CAD-JSON:\n" + JSON.stringify(cad)
    }
  ];

  const requestBody = {
    model: String(env.OPENAI_MODEL),
    instructions,
    input,
    tools,
    parallel_tool_calls: true,
    store: false,
    max_output_tokens: positiveInt(env.OPENAI_MAX_OUTPUT_TOKENS, 3200, 512, 12000)
  };
  if (allowWeb) requestBody.include = ["web_search_call.action.sources"];

  const fetcher = typeof env.__fetch === "function" ? env.__fetch : fetch;
  let upstream;
  try {
    upstream = await fetcher(OPENAI_RESPONSES_URL, {
      method: "POST",
      headers: {
        authorization: "Bearer " + String(env.OPENAI_API_KEY),
        "content-type": "application/json",
        accept: "application/json"
      },
      body: JSON.stringify(requestBody)
    });
  } catch (_) {
    return json({ status: "server_error", message: "OpenAI connection failed" }, 503);
  }

  let data;
  try {
    data = await upstream.json();
  } catch (_) {
    return json({ status: "server_error", message: "OpenAI returned invalid JSON" }, 502);
  }
  if (!upstream.ok) {
    const upstreamMessage = data && data.error && data.error.message ? String(data.error.message) : "OpenAI request failed";
    return json({ status: "server_error", message: upstreamMessage.slice(0, 300) }, upstream.status >= 500 ? 503 : 502);
  }

  const parsedOutput = parseOpenAiOutput(data);
  const reply = parsedOutput.reply || (parsedOutput.actions.length
    ? "Gandalf AI çizim için " + parsedOutput.actions.length + " adet düzenleme önerisi hazırladı. Bu işlemler henüz uygulanmadı."
    : "Gandalf AI yanıt üretemedi.");

  return json({
    status: "ok",
    reply,
    actions: parsedOutput.actions,
    sources: parsedOutput.sources,
    webUsed: parsedOutput.webUsed,
    sessionExpiresAtMs: session.expiresAtMs,
    accessMode
  });
}

function cadProposalTools() {
  return [
    functionTool(
      "cad_highlight_entities",
      "Propose highlighting existing CAD entities for user review. Does not modify the drawing.",
      {
        sourceIds: { type: "array", items: { type: "integer" }, minItems: 1, maxItems: 200 },
        reason: { type: "string" }
      },
      ["sourceIds", "reason"]
    ),
    functionTool(
      "cad_move_entity",
      "Propose moving one existing entity by a drawing-unit delta.",
      {
        sourceId: { type: "integer" },
        dx: { type: "number" },
        dy: { type: "number" },
        reason: { type: "string" }
      },
      ["sourceId", "dx", "dy", "reason"]
    ),
    functionTool(
      "cad_delete_entity",
      "Propose deleting one existing CAD entity.",
      {
        sourceId: { type: "integer" },
        reason: { type: "string" }
      },
      ["sourceId", "reason"]
    ),
    functionTool(
      "cad_change_layer",
      "Propose moving one existing CAD entity to another layer.",
      {
        sourceId: { type: "integer" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["sourceId", "layer", "reason"]
    ),
    functionTool(
      "cad_add_line",
      "Propose adding a line using active drawing coordinates.",
      {
        x1: { type: "number" }, y1: { type: "number" },
        x2: { type: "number" }, y2: { type: "number" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["x1", "y1", "x2", "y2", "layer", "reason"]
    ),
    functionTool(
      "cad_add_text",
      "Propose adding a CAD text note at a drawing coordinate.",
      {
        x: { type: "number" }, y: { type: "number" },
        text: { type: "string" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["x", "y", "text", "layer", "reason"]
    ),
    functionTool(
      "cad_replace_text",
      "Propose replacing the content of an existing text entity.",
      {
        sourceId: { type: "integer" },
        text: { type: "string" },
        reason: { type: "string" }
      },
      ["sourceId", "text", "reason"]
    )
  ];
}

function functionTool(name, description, properties, required) {
  return {
    type: "function",
    name,
    description,
    parameters: {
      type: "object",
      properties,
      required,
      additionalProperties: false
    },
    strict: true
  };
}

function parseOpenAiOutput(data) {
  const texts = [];
  const actions = [];
  const sourceMap = new Map();
  let webUsed = false;
  const output = data && Array.isArray(data.output) ? data.output : [];

  for (const item of output) {
    if (!item || typeof item !== "object") continue;
    if (item.type === "web_search_call") {
      webUsed = true;
      const sources = item.action && Array.isArray(item.action.sources) ? item.action.sources : [];
      for (const source of sources) {
        const url = String(source && source.url || "").trim();
        if (!/^https?:\/\//i.test(url)) continue;
        const title = String(source && source.title || "").trim();
        sourceMap.set(url, { title, url });
      }
    }

    if (item.type === "function_call") {
      const name = String(item.name || "").trim();
      if (!name) continue;
      let args = {};
      try { args = JSON.parse(String(item.arguments || "{}")); } catch (_) { args = {}; }
      actions.push({
        name,
        arguments: args,
        reason: typeof args.reason === "string" ? args.reason : ""
      });
      continue;
    }

    if (item.type === "message" && Array.isArray(item.content)) {
      for (const part of item.content) {
        if (part && part.type === "output_text" && typeof part.text === "string" && part.text.trim()) {
          texts.push(part.text.trim());
          const annotations = Array.isArray(part.annotations) ? part.annotations : [];
          for (const annotation of annotations) {
            if (!annotation || annotation.type !== "url_citation") continue;
            const citation = annotation.url_citation && typeof annotation.url_citation === "object"
              ? annotation.url_citation : annotation;
            const url = String(citation.url || "").trim();
            if (!/^https?:\/\//i.test(url)) continue;
            const title = String(citation.title || "").trim();
            sourceMap.set(url, { title, url });
          }
        }
      }
    }
  }

  return {
    reply: texts.join("\n\n").trim(),
    actions,
    sources: Array.from(sourceMap.values()).slice(0, 20),
    webUsed
  };
}

async function verifySession(authHeader, publicKeyPem) {
  try {
    const match = /^Bearer\s+(.+)$/i.exec(String(authHeader || ""));
    if (!match) return { ok: false, message: "Missing AI session" };
    const token = match[1].trim();
    const parts = token.split(".");
    if (parts.length !== 3 || (parts[0] !== "MAI1" && parts[0] !== "MAI2"))
      return { ok: false, message: "Invalid AI session" };

    const payloadBytes = decodeBase64url(parts[1]);
    const signatureBytes = decodeBase64url(parts[2]);
    const payload = new TextDecoder().decode(payloadBytes);
    const fields = payload.split("|");

    let mode = "licensed";
    if (parts[0] === "MAI1") {
      if (fields.length !== 3 || fields[0] !== "MAI1")
        return { ok: false, message: "Invalid AI session payload" };
    } else {
      if (fields.length !== 4 || fields[0] !== "MAI2")
        return { ok: false, message: "Invalid AI session payload" };
      mode = fields[3] === "developer" ? "developer" : "licensed";
    }

    const deviceId = fields[1];
    const expiresAtMs = Number(fields[2]);
    if (!validDeviceId(deviceId) || !Number.isSafeInteger(expiresAtMs) || expiresAtMs <= Date.now())
      return { ok: false, message: "AI session expired" };

    const key = await crypto.subtle.importKey(
      "spki",
      pemBytes(publicKeyPem, "PUBLIC KEY"),
      { name: "RSASSA-PKCS1-v1_5", hash: "SHA-256" },
      false,
      ["verify"]
    );
    const valid = await crypto.subtle.verify("RSASSA-PKCS1-v1_5", key, signatureBytes, payloadBytes);
    return valid ? { ok: true, deviceId, expiresAtMs, mode } : { ok: false, message: "Invalid AI session signature" };
  } catch (_) {
    return { ok: false, message: "Invalid AI session" };
  }
}

async function readJson(request) {
  try {
    const text = await request.text();
    if (new TextEncoder().encode(text).length > MAX_BODY_BYTES)
      return { ok: false, response: json({ status: "denied", message: "request too large" }, 413) };
    return { ok: true, body: JSON.parse(text) };
  } catch (_) {
    return { ok: false, response: json({ status: "denied", message: "invalid json" }, 400) };
  }
}

function validDeviceId(value) {
  return /^MC-[0-9A-F]{8}-[0-9A-F]{8}-[0-9A-F]{8}$/.test(String(value || ""))
    || /^MC-FALLBACK-[0-9A-F-]{36}$/.test(String(value || ""));
}

function positiveInt(value, fallback, min, max) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) return fallback;
  return Math.max(min, Math.min(max, Math.floor(parsed)));
}

function pemBytes(pem, label) {
  const normalized = String(pem || "").replace(/\\n/g, "\n");
  const clean = normalized
    .replace(`-----BEGIN ${label}-----`, "")
    .replace(`-----END ${label}-----`, "")
    .replace(/\s/g, "");
  if (!clean) throw new Error("missing key");
  const binary = atob(clean);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) out[i] = binary.charCodeAt(i);
  return out;
}

function decodeBase64url(value) {
  const normalized = String(value || "").replace(/-/g, "+").replace(/_/g, "/");
  const padded = normalized + "=".repeat((4 - normalized.length % 4) % 4);
  const binary = atob(padded);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) out[i] = binary.charCodeAt(i);
  return out;
}

function json(body, status = 200, extraHeaders = {}) {
  return new Response(JSON.stringify(body), {
    status,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "cache-control": "no-store",
      "x-content-type-options": "nosniff",
      ...extraHeaders
    }
  });
}

export { parseOpenAiOutput, cadProposalTools, verifySession };
