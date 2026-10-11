# MusaCAD — Google Drive storage backend (R2 replacement proposal)

Status: DESIGN ONLY; not deployed, not implemented. PR #233 still uses private R2 and Durable Objects. Do not set MUSACAD_AUTOUPLOAD_ENABLED=true until a Drive adapter is implemented and integration-tested.

## Decision (2026-10-11)
Use a user's private Google Drive storage for DWG/DXF project backups instead of provisioning a paid Cloudflare R2 subscription. Keep the existing signed-device upload API and Gandalf analysis server. Google Drive is not an S3/R2 drop-in replacement: the storage implementation, authorization, quota and cleanup must change.

## Security / consent
- Android OAuth client com.musa.cad and its SHA-1 already configured in Google Cloud; this does NOT authorize Drive access by itself.
- Configure Google Drive API, request the minimum suitable Drive scope (prefer drive.file for app-created files), and implement Google Sign-In/authorization on the device. Request consent once per account, allow revoke/disconnect and deletion. Test users must be enrolled while OAuth is in Testing.
- Do not put OAuth refresh tokens, access tokens, service-account JSON or credentials in GitHub, APK constants or logs.
- The Cloudflare Worker cannot use an Android OAuth client as a server credential. Use a dedicated secure server-side OAuth flow/token exchange with encrypted token storage and rotation, OR have the Android client upload directly with short-lived user access tokens and send a signed manifest to the server. Decide after threat-model review.
- Never grant another device/user access based on a caller-provided Drive file ID. Bind each manifest to the authenticated user and signed MusaCAD device identity.
- Prefer a dedicated private app folder and opaque object names. Do not publish files with link-sharing permissions. Respect municipal/employer confidentiality policies.

## Compatibility contract
Preserve POST /v1/projects/init, PUT /v1/projects/:id/chunks/:part, POST /v1/projects/:id/complete, POST /v1/projects/sync-open, GET /v1/projects/status and index pages semantics, including per-chunk SHA-256, monotonic revision, resumability and atomic completion. Replace R2 put/get/list/delete with a storage abstraction. Drive resumable uploads are a different protocol: maintain verified staging state, durable chunk manifest and safe retries, and never claim 100% until the Drive file and manifest are confirmed.

Drive API may throttle and has storage quota. Distinguish storage-full, expired OAuth, revoked permission, rate limit, and network failures; use bounded exponential backoff. Never silently fall back to public sharing or delete local drawings.

## Multi-project retention
Maintain all up-to-four OPEN projects. Preserve the last complete closed project until a replacement is verified. Delete only owned app-created Drive files after successful replacement and confirmed open-set revision; cleanup must be idempotent. Add a user-visible Delete all cloud copies action, audit without content or secrets, and show storage consumed.

## Backend coordination
The existing PR #233 Durable Object coordinates per-device revisions; replacing R2 alone does NOT eliminate Durable Object requirements. Choose a persistent transactional catalog (e.g. Cloudflare D1 or a suitable alternative) and test concurrent revisions, reconnect, and stale close events before removing DO. Do not assume Google Drive folders or metadata provide atomic compare-and-swap.

## Acceptance tests before any production change
1. Android OAuth sign-in, scope consent, token expiry/revocation and separate accounts.
2. Real Drive upload of a DWG, SHA verification, interrupted transfer resume and 100% only on durable completion.
3. Four simultaneous drawings; close B while A/C/D remain; no cross-account leakage.
4. Storage quota, 429, 5xx and lost network; no local deletion or false success.
5. App disconnect and explicit delete-cloud-copies, verified cleanup.
6. AI reads only authorized complete project indexes; incomplete visual/engineering analysis never reported as complete.
7. Real phone and CI regression pass; separate PR review. No live deployment, APK publication or merge based solely on this document.
