const TRIAL_MS = 24 * 60 * 60 * 1000;
const MAX_BODY_BYTES = 8192;
const GOOGLE_OAUTH_URL = "https://oauth2.googleapis.com/token";
const ANDROID_PUBLISHER_SCOPE = "https://www.googleapis.com/auth/androidpublisher";

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === "/v1/trial/start") return handleTrial(request, env);
    if (url.pathname === "/v1/play/verify") return handlePlayVerify(request, env);
    return json({ status: "not_found" }, 404);
  }
};

async function handleTrial(request, env) {
  if (request.method !== "POST") return json({ status: "method_not_allowed" }, 405, { Allow: "POST" });
  if (!env.DB || !env.MUSACAD_TRIAL_PRIVATE_KEY_PEM) return json({ status: "server_error" }, 503);

  const parsed = await readJson(request);
  if (!parsed.ok) return parsed.response;
  const body = parsed.body;

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

async function handlePlayVerify(request, env) {
  if (request.method !== "POST") return json({ status: "method_not_allowed" }, 405, { Allow: "POST" });
  if (!env.DB || !env.MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL || !env.MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM)
    return json({ status: "server_error", message: "Google Play server credentials are not configured" }, 503);

  const parsed = await readJson(request);
  if (!parsed.ok) return parsed.response;
  const body = parsed.body;

  const deviceId = String(body.deviceId || "").trim().toUpperCase();
  const packageName = String(body.packageName || "").trim();
  const productId = String(body.productId || "").trim();
  const purchaseToken = String(body.purchaseToken || "").trim();
  const purpose = String(body.purpose || "").trim();
  const allowedPackage = env.MUSACAD_PACKAGE_NAME || "com.musa.cad";
  const allowedProduct = env.MUSACAD_PLAY_YEARLY_PRODUCT_ID || "musacad_yearly_renewal";

  if (!validDeviceId(deviceId)
      || packageName !== allowedPackage
      || productId !== allowedProduct
      || purpose !== "annual_renewal"
      || purchaseToken.length < 10
      || purchaseToken.length > 4096) {
    return json({ status: "denied", message: "Invalid purchase binding" }, 403);
  }

  const fetcher = typeof env.__fetch === "function" ? env.__fetch : fetch;
  try {
    const accessToken = await googleAccessToken(env, fetcher);
    const purchase = await fetchGoogleSubscription(fetcher, accessToken, packageName, purchaseToken);
    if (purchase.errorStatus) {
      if (purchase.errorStatus >= 500) return json({ status: "server_error", message: "Google Play verification unavailable" }, 503);
      return json({ status: "denied", message: "Google Play yearly renewal was not found" }, 403);
    }

    const data = purchase.data || {};
    const state = String(data.subscriptionState || "");
    if (state.includes("PENDING")) return json({ status: "pending", message: "Payment is still pending" }, 202);
    if (state !== "SUBSCRIPTION_STATE_ACTIVE" && state !== "SUBSCRIPTION_STATE_IN_GRACE_PERIOD")
      return json({ status: "denied", message: "Yearly renewal is not active" }, 403);

    const lineItems = Array.isArray(data.lineItems) ? data.lineItems : [];
    if (!lineItems.some(item => item && item.productId === productId))
      return json({ status: "denied", message: "Subscription product does not match MusaCAD yearly renewal" }, 403);

    const accountId = String(
      data.externalAccountIdentifiers && data.externalAccountIdentifiers.obfuscatedExternalAccountId || ""
    ).trim().toUpperCase();
    const allowLegacyUnbound = String(env.MUSACAD_PLAY_ALLOW_LEGACY_UNBOUND || "").toLowerCase() === "true";
    if (accountId) {
      if (accountId !== deviceId) return json({ status: "denied", message: "Purchase belongs to another MusaCAD device" }, 409);
    } else if (!allowLegacyUnbound) {
      return json({ status: "denied", message: "Purchase is missing MusaCAD device binding" }, 403);
    }

    const tokenHash = await sha256Hex(purchaseToken);
    const existing = await env.DB.prepare(
      "SELECT device_id,package_name,product_id FROM play_purchases WHERE token_hash=? LIMIT 1"
    ).bind(tokenHash).first();
    if (existing && (existing.device_id !== deviceId
        || existing.package_name !== packageName
        || existing.product_id !== productId)) {
      return json({ status: "denied", message: "Purchase token was already bound to another device" }, 409);
    }

    const now = Date.now();
    const expiryTimes = lineItems
      .map(item => Date.parse(String(item && item.expiryTime || "")))
      .filter(value => Number.isFinite(value));
    const expiresAtMs = expiryTimes.length ? Math.max(...expiryTimes) : 0;
    if (!expiresAtMs || expiresAtMs <= now)
      return json({ status: "denied", message: "Yearly renewal has expired" }, 403);

    const orderId = lineItems.map(item => item && item.latestSuccessfulOrderId).find(Boolean) || "";
    await env.DB.prepare(
      "INSERT OR IGNORE INTO play_purchases(token_hash,device_id,package_name,product_id,order_id,verified_at_ms) VALUES(?,?,?,?,?,?)"
    ).bind(tokenHash, deviceId, packageName, productId, String(orderId), now).run();

    if (data.acknowledgementState !== "ACKNOWLEDGEMENT_STATE_ACKNOWLEDGED") {
      const acknowledged = await acknowledgeGoogleSubscription(fetcher, accessToken, packageName, productId, purchaseToken);
      if (!acknowledged) {
        const refreshed = await fetchGoogleSubscription(fetcher, accessToken, packageName, purchaseToken);
        if (refreshed.errorStatus
            || !refreshed.data
            || refreshed.data.acknowledgementState !== "ACKNOWLEDGEMENT_STATE_ACKNOWLEDGED") {
          return json({ status: "server_error", message: "Yearly renewal verified but acknowledgement failed" }, 503);
        }
      }
    }

    await env.DB.prepare(
      "UPDATE play_purchases SET verified_at_ms=?,acknowledged_at_ms=? WHERE token_hash=?"
    ).bind(now, now, tokenHash).run();

    return json({ status: "active", productId, purpose: "annual_renewal", expiresAtMs, acknowledged: true }, 200);
  } catch (_) {
    return json({ status: "server_error", message: "Google Play verification failed" }, 503);
  }
}

async function googleAccessToken(env, fetcher) {
  const now = Math.floor(Date.now() / 1000);
  const header = base64url(new TextEncoder().encode(JSON.stringify({ alg: "RS256", typ: "JWT" })));
  const claims = base64url(new TextEncoder().encode(JSON.stringify({
    iss: String(env.MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL),
    scope: ANDROID_PUBLISHER_SCOPE,
    aud: GOOGLE_OAUTH_URL,
    iat: now,
    exp: now + 3600
  })));
  const unsigned = header + "." + claims;
  const key = await crypto.subtle.importKey(
    "pkcs8",
    pemBytes(env.MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM, "PRIVATE KEY"),
    { name: "RSASSA-PKCS1-v1_5", hash: "SHA-256" },
    false,
    ["sign"]
  );
  const signature = await crypto.subtle.sign(
    "RSASSA-PKCS1-v1_5",
    key,
    new TextEncoder().encode(unsigned)
  );
  const assertion = unsigned + "." + base64url(new Uint8Array(signature));
  const response = await fetcher(GOOGLE_OAUTH_URL, {
    method: "POST",
    headers: { "content-type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams({
      grant_type: "urn:ietf:params:oauth:grant-type:jwt-bearer",
      assertion
    }).toString()
  });
  if (!response.ok) throw new Error("oauth");
  const body = await response.json();
  if (!body || !body.access_token) throw new Error("oauth token");
  return String(body.access_token);
}

async function fetchGoogleSubscription(fetcher, accessToken, packageName, purchaseToken) {
  const url = "https://androidpublisher.googleapis.com/androidpublisher/v3/applications/"
    + encodeURIComponent(packageName)
    + "/purchases/subscriptionsv2/tokens/"
    + encodeURIComponent(purchaseToken);
  const response = await fetcher(url, {
    method: "GET",
    headers: { authorization: "Bearer " + accessToken, accept: "application/json" }
  });
  if (!response.ok) return { errorStatus: response.status };
  return { data: await response.json() };
}

async function acknowledgeGoogleSubscription(fetcher, accessToken, packageName, productId, purchaseToken) {
  const url = "https://androidpublisher.googleapis.com/androidpublisher/v3/applications/"
    + encodeURIComponent(packageName)
    + "/purchases/subscriptions/"
    + encodeURIComponent(productId)
    + "/tokens/"
    + encodeURIComponent(purchaseToken)
    + ":acknowledge";
  const response = await fetcher(url, {
    method: "POST",
    headers: {
      authorization: "Bearer " + accessToken,
      accept: "application/json",
      "content-type": "application/json"
    },
    body: "{}"
  });
  return response.ok;
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
  return /^MC-[0-9A-F]{8}-[0-9A-F]{8}-[0-9A-F]{8}$/.test(value)
    || /^MC-FALLBACK-[0-9A-F-]{36}$/.test(value);
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

async function sha256Hex(value) {
  const bytes = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(value));
  return Array.from(new Uint8Array(bytes), b => b.toString(16).padStart(2, "0")).join("");
}

function pemBytes(pem, label) {
  const normalized = String(pem || "").replace(/\\n/g, "\n");
  const clean = normalized
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
