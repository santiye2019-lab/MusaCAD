# MusaCAD Gandalf Gateway

This small service sits between the Android app and the OpenAI API.

## Why a gateway exists

- The OpenAI API key is **never embedded in the APK**.
- MusaCAD sends only the privacy-minimized project packet, not device IDs,
  license secrets or absolute file paths.
- OpenAI responses are requested with `store: false`.
- CAD edit suggestions are filtered on the gateway and are filtered again on
  Android. Editing commands still require explicit user approval in MusaCAD.
- ChatGPT identity or a ChatGPT Plus/Pro subscription is **not** treated as an
  API credential or cloud entitlement. Production account/entitlement
  verification must be implemented as a separate supported authentication
  flow.

## Environment

- `OPENAI_API_KEY` — server-side OpenAI API key.
- `OPENAI_MODEL` — optional; defaults to `gpt-5.6-sol`.
- `MUSACAD_GATEWAY_BEARER_TOKEN` — development/app-to-gateway bearer token.
  For production, replace this static token with short-lived per-user sessions.
- `PORT` — optional, defaults to 8787.

## Endpoints

### GET /health

Returns gateway health and selected model. It never returns secrets.

### POST /analyze

Requires:

`Authorization: Bearer <MusaCAD session token>`

Request:

```json
{
  "schema": "musacad.gandalf.request.v1",
  "prompt": "Bu projeyi mekanik açıdan incele",
  "project": {
    "schema": "musacad.project.v1"
  }
}
```

Response:

```json
{
  "reply": "Teknik analiz...",
  "actions": [
    {
      "command": "ZE",
      "description": "Çizimi ekrana sığdır"
    }
  ]
}
```

The Android app does **not** blindly execute `actions`. It applies its own
command allowlist and asks the user before any drawing/file mutation.

## Android build setting

Set `MUSACAD_AI_GATEWAY_URL` to the full HTTPS analyze endpoint, for example:

`https://ai.example.com/analyze`

The current Android client rejects non-HTTPS gateway URLs.
