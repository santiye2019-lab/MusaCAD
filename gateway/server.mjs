import http from "node:http";
import { randomBytes } from "node:crypto";

const PORT = Number(process.env.PORT || 8787);
const OPENAI_API_KEY = (process.env.OPENAI_API_KEY || "").trim();
const MODEL = (process.env.OPENAI_MODEL || "gpt-5.6-sol").trim();
const CLIENT_BEARER = (process.env.MUSACAD_GATEWAY_BEARER_TOKEN || "").trim();
const PAIRING_CODE = (process.env.MUSACAD_PAIRING_CODE || "").trim();
const SESSION_TTL_MS = Math.max(15 * 60_000, Number(process.env.MUSACAD_SESSION_TTL_MS || 2 * 60 * 60_000));
const SESSIONS = new Map();

const ALLOWED_VERBS = new Set([
  "ZE","ZOOM","PAN","3D","2D","LA","PR","DI","AA","ANG","ID","ARCLEN","HELP","LIST",
  "SELECT","MOVE","COPY","ERASE","ROTATE","SCALE","MIRROR","OFFSET","ARRAY","EXPLODE",
  "TRIM","EXTEND","FILLET","CHAMFER","BREAK","PEDIT","MATCHPROP","JOIN","HATCH","STRETCH",
  "BLOCK","INSERT","DIVIDE","REVCLOUD","MLEADER","PLINE","XLINE","LINE","CIRCLE","ARC",
  "ELLIPSE","POINT","RECTANG","TEXT","DLI","DAL","DAN","DRA","DDI","UNDO","REDO","SAVE","QSAVE"
]);

const OUTPUT_SCHEMA = {
  type: "object",
  properties: {
    reply: {
      type: "string",
      description: "Turkish engineering analysis or answer for the user."
    },
    actions: {
      type: "array",
      maxItems: 12,
      items: {
        type: "object",
        properties: {
          command: { type: "string" },
          description: { type: "string" }
        },
        required: ["command", "description"],
        additionalProperties: false
      }
    }
  },
  required: ["reply", "actions"],
  additionalProperties: false
};

const INSTRUCTIONS = `
You are Gandalf inside MusaCAD, an engineering CAD assistant.
Answer in Turkish unless the user explicitly asks for another language.

The request contains a privacy-minimized MusaCAD project packet with drawing
layers, object types, text, geometry measurements and a mechanical summary.

Rules:
1. Base drawing-specific claims on the supplied project packet. Clearly say
   when the drawing data is insufficient.
2. For mechanical-installation review, identify concrete findings, missing
   labels/data, coordination risks, continuity issues and items worth checking.
3. Do not claim that a drawing officially complies with a regulation merely
   because no issue was detected.
4. Never invent coordinates, dimensions, diameters, capacities, device counts,
   layers or objects that are absent from the packet.
5. If a CAD edit would help, return it only as an action suggestion. Never
   describe an action as already executed.
6. Keep actions small, reversible and directly tied to the user's request.
7. Use only MusaCAD canonical CAD commands. The Android app will independently
   validate every command and requires user approval for edits.
8. Prefer analysis/reporting over edits when the correct geometry or target is
   ambiguous.
`.trim();

function json(res, status, body) {
  const data = Buffer.from(JSON.stringify(body), "utf8");
  res.writeHead(status, {
    "content-type": "application/json; charset=utf-8",
    "content-length": data.length,
    "cache-control": "no-store"
  });
  res.end(data);
}

function authorized(req) {
  const header = String(req.headers.authorization || "");
  if (!header.startsWith("Bearer ")) return false;
  const token = header.slice("Bearer ".length).trim();
  if (!token) return false;
  if (CLIENT_BEARER && token === CLIENT_BEARER) return true;
  const expires = SESSIONS.get(token);
  if (!expires) return false;
  if (expires <= Date.now()) {
    SESSIONS.delete(token);
    return false;
  }
  return true;
}

function createSession() {
  const token = randomBytes(32).toString("base64url");
  const expiresAt = Date.now() + SESSION_TTL_MS;
  SESSIONS.set(token, expiresAt);
  return { token, expiresAt };
}

async function readJson(req) {
  const chunks = [];
  let size = 0;
  for await (const chunk of req) {
    size += chunk.length;
    if (size > 2_000_000) throw new Error("request_too_large");
    chunks.push(chunk);
  }
  const raw = Buffer.concat(chunks).toString("utf8");
  return JSON.parse(raw || "{}");
}

function extractOutputText(response) {
  const pieces = [];
  for (const item of response?.output || []) {
    if (item?.type !== "message") continue;
    for (const part of item?.content || []) {
      if (part?.type === "output_text" && typeof part.text === "string") pieces.push(part.text);
    }
  }
  return pieces.join("\n").trim();
}

function filterActions(actions) {
  if (!Array.isArray(actions)) return [];
  const out = [];
  for (const item of actions.slice(0, 12)) {
    const command = String(item?.command || "").trim().toUpperCase();
    if (!command) continue;
    const verb = command.split(/\s+/, 1)[0];
    if (!ALLOWED_VERBS.has(verb)) continue;
    out.push({
      command: command.slice(0, 160),
      description: String(item?.description || "").trim().slice(0, 500)
    });
  }
  return out;
}

async function callOpenAI(prompt, project) {
  if (!OPENAI_API_KEY) throw new Error("OPENAI_API_KEY_not_configured");

  const input = JSON.stringify({
    user_request: prompt,
    project
  });

  const apiResponse = await fetch("https://api.openai.com/v1/responses", {
    method: "POST",
    headers: {
      "authorization": `Bearer ${OPENAI_API_KEY}`,
      "content-type": "application/json"
    },
    body: JSON.stringify({
      model: MODEL,
      store: false,
      reasoning: { effort: "medium" },
      instructions: INSTRUCTIONS,
      input,
      text: {
        format: {
          type: "json_schema",
          name: "musacad_gandalf_response",
          strict: true,
          schema: OUTPUT_SCHEMA
        }
      }
    })
  });

  const raw = await apiResponse.text();
  if (!apiResponse.ok) {
    throw new Error(`openai_http_${apiResponse.status}: ${raw.slice(0, 500)}`);
  }

  const response = JSON.parse(raw);
  const outputText = extractOutputText(response);
  if (!outputText) throw new Error("openai_empty_response");

  const parsed = JSON.parse(outputText);
  return {
    reply: String(parsed.reply || "").trim(),
    actions: filterActions(parsed.actions)
  };
}

const server = http.createServer(async (req, res) => {
  try {
    if (req.method === "GET" && req.url === "/health") {
      return json(res, 200, {
        ok: true,
        service: "musacad-gandalf-gateway",
        model: MODEL,
        openai_configured: Boolean(OPENAI_API_KEY),
        pairing_configured: Boolean(PAIRING_CODE)
      });
    }

    if (req.method === "POST" && req.url === "/session") {
      if (!PAIRING_CODE) return json(res, 503, { error: "pairing_not_configured" });
      const body = await readJson(req);
      const supplied = String(body?.pairing_code || "").trim();
      if (!supplied || supplied !== PAIRING_CODE) {
        return json(res, 401, { error: "invalid_pairing_code" });
      }
      const session = createSession();
      return json(res, 200, {
        token: session.token,
        expires_at_ms: session.expiresAt,
        entitlement_verified: true,
        source: "gateway_pairing_dev"
      });
    }

    if (req.method !== "POST" || req.url !== "/analyze") {
      return json(res, 404, { error: "not_found" });
    }

    if (!authorized(req)) {
      return json(res, 401, { error: "unauthorized" });
    }

    const body = await readJson(req);
    const prompt = String(body?.prompt || "").trim();
    const project = body?.project;

    if (!prompt || prompt.length > 8000) {
      return json(res, 400, { error: "invalid_prompt" });
    }
    if (!project || project.schema !== "musacad.project.v1") {
      return json(res, 400, { error: "invalid_project_packet" });
    }

    const result = await callOpenAI(prompt, project);
    return json(res, 200, result);
  } catch (error) {
    const message = String(error?.message || "gateway_error");
    const status = message === "request_too_large" ? 413 : 500;
    return json(res, status, { error: "gateway_error", detail: message.slice(0, 500) });
  }
});

server.listen(PORT, "0.0.0.0", () => {
  console.log(`MusaCAD Gandalf gateway listening on :${PORT}`);
});
