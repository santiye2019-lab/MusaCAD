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
- optional `AI_MAX_OUTPUT_TOKENS=3200`

The Gemini route uses Google's OpenAI-compatible Chat Completions endpoint so MusaCAD can retain its bounded CAD function-calling surface.

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
