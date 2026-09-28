# MusaCAD AI Worker

Secure server-side gateway for **Gandalf AI** inside MusaCAD.

## Security model

- The OpenAI API key exists only as the Worker secret `OPENAI_API_KEY`.
- Android never receives the OpenAI API key.
- `/v1/analyze` requires a short-lived `MAI1` bearer token issued by the MusaCAD license/trial worker.
- The token is device-bound and expires after 15 minutes.
- Raw DWG/DXF files are rejected by policy; the Android client sends bounded `musacad-cad-json/v1` metadata.
- CAD function calls are returned to the app as **proposed actions** only. They are not automatically executed.

## Required secrets / variables

Secrets:
- `OPENAI_API_KEY`
- `MUSACAD_AI_SESSION_PUBLIC_KEY_PEM` (same public key corresponding to the trial worker's AI-session signing private key)

Variables:
- `OPENAI_MODEL=<supported Responses API model>`
- optional `OPENAI_MAX_OUTPUT_TOKENS=3200`

## Endpoints

- `GET /health`
- `POST /v1/analyze`

The Android build uses:
- `MUSACAD_AI_SESSION_URL=https://<license-worker>/v1/ai/session`
- `MUSACAD_AI_API_URL=https://<ai-worker>/v1/analyze`

The Responses API request may expose OpenAI web search only when the user explicitly asks for current/web/source-based analysis. CAD editing tools are exposed only when the user's prompt explicitly asks to modify or revise the drawing.
