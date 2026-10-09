# MusaCAD AI Worker

Secure server-side gateway for **Gandalf AI** inside MusaCAD.

## Security model

- The Gemini API key exists only as the Worker secret `GEMINI_API_KEY`.
- Android never receives the Gemini API key.
- `/v1/analyze` requires a short-lived `MAI1` or `MAI2` bearer token issued by the MusaCAD license/trial worker.
- The token is device-bound and expires after 15 minutes.
- Raw DWG/DXF files are rejected by policy; the Android client sends bounded `musacad-cad-json/v1` metadata.
- CAD function calls are returned to the app as **proposed actions** only. They are not automatically executed.
- The gateway can still run with OpenAI if `AI_PROVIDER=openai` and the legacy OpenAI secret/model are configured, but production deployment defaults to Gemini.

## Gemini Free Tier configuration

Secrets:
- `GEMINI_API_KEY`
- `MUSACAD_AI_SESSION_PUBLIC_KEY_PEM` (same public key corresponding to the trial worker's AI-session signing private key)

Variables:
- `AI_PROVIDER=gemini`
- `GEMINI_MODEL=gemini-3.8-flash`
- `GEMINI_API_MODE=native` (recommended for production)
- optional `AI_MAX_OUTPUT_TOKENS=3200`

The production deployment uses Google's **native Gemini generateContent API** (`GEMINI_API_MODE=native`), which supports image evidence and function declarations. It avoids failures observed on the OpenAI-compatible Chat Completions endpoint while keeping CAD edits as explicit proposals only. The older Chat Completions integration remains available with `GEMINI_API_MODE=chat` for compatibility.

When Gemini Free Tier returns HTTP 429, the gateway returns `quota_exhausted` with a clear message. No paid provider is invoked automatically.

Live web-search grounding is not exposed by the Free Tier route. If a request asks for current web verification, Gandalf must state that live web verification is unavailable rather than pretending it searched.

## Optional OpenAI fallback configuration

If intentionally switching back to OpenAI:

Secrets:
- `OPENAI_API_KEY`

Variables:
- `AI_PROVIDER=openai`
- `OPENAI_MODEL=<supported Responses API model>`
- optional `AI_MAX_OUTPUT_TOKENS=3200`

## Endpoints

- `GET /health`
- `POST /v1/analyze`

The Android build uses:
- `MUSACAD_AI_SESSION_URL=https://<license-worker>/v1/ai/session`
- `MUSACAD_AI_API_URL=https://<ai-worker>/v1/analyze`

The worker returns provider/model metadata and token usage when the upstream provider reports it. CAD editing tools are exposed only when the user's prompt explicitly asks to modify or revise the drawing.

## Optional self-hosted Qwen vision provider (no Gemini API quota)

This is **not** a free hosted AI API. A project owner must first provision and
operate a computer/server with enough RAM/VRAM. The Qwen model software can
run without per-request Gemini API charges; your compute, electricity and
host capacity still have real costs and technical limits.

Supported upstream shape: OpenAI-compatible `POST /v1/chat/completions` with
text + data-URL JPEG images and optional tool proposals. Tested contract with
Ollama's Qwen3.5-4B (`qwen3.5:4b`); other compatible visual models may work
after testing. The Android app continues to send consented CAD-JSON and small
visual crops through the existing authenticated MusaCAD Worker.

When choosing **selfhosted** in GitHub Actions "Deploy Gandalf AI and build APK",
supply repository secrets:

- `MUSACAD_SELFHOSTED_AI_ENDPOINT`: external **HTTPS** URL ending exactly
  `/v1/chat/completions` (a protected reverse-proxy/tunnel, not localhost)
- `MUSACAD_SELFHOSTED_AI_API_KEY`: reverse-proxy **bearer credential**,
  distinct from the existing license or Gemini secrets.

Optional repository variable:
`MUSACAD_SELFHOSTED_MODEL=qwen3.5:4b` (default).

The workflow checks a tiny upstream completion **before** deploying to
Cloudflare. If the server is unavailable, nothing is deployed. Selecting
`gemini` retains the existing production Gemini path and is the default.
The license DB and entitlement Worker are untouched. **Never expose an
unauthenticated Ollama port (11434) to the internet.** Configure HTTPS,
bearer validation, restrictive firewall and rate/concurrency controls on
your own reverse proxy. The Cloudflare Worker verifies the existing signed
license session before accessing the inference server.

The cloud Worker accepts `AI_PROVIDER=selfhosted`, `SELFHOSTED_MODEL`,
`SELFHOSTED_AI_ENDPOINT` (secret) and `SELFHOSTED_AI_API_KEY` (secret).
No account, endpoint or authentication details are embedded in the APK.
A `429` from the operator's own gateway is a *capacity/rate* limit, not
Gemini Free Tier quota. Requests are never silently billed through Gemini.

Operational and acceptance details: `docs/MUSACAD_QWEN_SELFHOSTED_AI.md`.
