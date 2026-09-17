const TRIAL_MS = 24 * 60 * 60 * 1000;
const MAX_BODY_BYTES = 8192;

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname !== "/v1/trial/start") return json({ status: "not_found" }, 404);
    if (request.method !== "POST") return json({ status: "method_not_allowed" }, 405, { Allow: "POST" });
    if (!env.DB || !env.MUSACAD_TRIAL_PRIVATE_KEY_PEM) return json({ status: "server_error" }, 503);

    let body;
    try {
      const text = await request.text();
      if (new TextEncoder().encode(text).length > MAX_BODY_BYTES) return json({ status: "denied", message: "request too large" }, 413);
      body = JSON.parse(text);
    } catch (_) {
      return json({ status: "denied", message: "invalid json" }, 400);
    }

    const deviceId = String(body.deviceId || "").trim().toUpperCase();
    const packageName = String(body.packageName || "").trim();
    const allowedPackage = env.MUSACAD_PACKAGE_NAME || "com.musa.cad";
    if (!validDeviceId(deviceId) || packageName !== allowedPackage) return json({ status: "denied" }, 403);

    const now = Date.now();
    const firstExpiry = now + TRIAL_MS;
    try {
      await env.DB.prepare(
        "INSERT OR IGNORE INTO trials(device_id,package_name,started_at_ms,expires_at_ms) VALUES(?,?,?,?)"
      ).bind(deviceId, packageName, now, firstExpiry).run();

      const row = await env.DB.prepare(
        "SELECT package_name,started_at_ms,expires_at_ms FROM trials WHERE device_id=? LIMIT 1"
      ).bind(deviceId).first();
      if (!row || row.package_name !== packageName) return json({ status: "denied" }, 403);

      const expiresAt = Number(row.expires_at_ms);
      if (!Number.isSafeInteger(expiresAt) || expiresAt <= now) return json({ status: "used" }, 409);
      const token = await signToken(deviceId, expiresAt, env.MUSACAD_TRIAL_PRIVATE_KEY_PEM);
      return json({ status: "active", token, expiresAtMs: expiresAt }, 200);
    } catch (_) {
      return json({ status: "server_error" }, 503);
    }
  }
};

function validDeviceId(value) {
  return /^MC-[0-9A-F]{8}-[0-9A-F]{8}-[0-9A-F]{8}$/.test(value) || /^MC-FALLBACK-[0-9A-F-]{36}$/.test(value);
}

async function signToken(deviceId, expiresAtMs, privateKeyPem) {
  const payload = `MT1|${deviceId}|${expiresAtMs}`;
  const key = await crypto.subtle.importKey(
    "pkcs8",
    pemBytes(privateKeyPem, "PRIVATE KEY"),
    { name: "RSASSA-PKCS1-v1_5", hash: "SHA-256" },
    false,
    ["sign"]
  );
  const payloadBytes = new TextEncoder().encode(payload);
  const signature = await crypto.subtle.sign("RSASSA-PKCS1-v1_5", key, payloadBytes);
  return `MT1.${base64url(payloadBytes)}.${base64url(new Uint8Array(signature))}`;
}

function pemBytes(pem, label) {
  const clean = String(pem || "")
    .replace(`-----BEGIN ${label}-----`, "")
    .replace(`-----END ${label}-----`, "")
    .replace(/\s/g, "");
  if (!clean) throw new Error("missing private key");
  const binary = atob(clean);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) out[i] = binary.charCodeAt(i);
  return out;
}

function base64url(bytes) {
  let binary = "";
  for (let i = 0; i < bytes.length; i += 0x8000) {
    binary += String.fromCharCode(...bytes.subarray(i, Math.min(i + 0x8000, bytes.length)));
  }
  return btoa(binary).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/g, "");
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
