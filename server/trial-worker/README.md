# MusaCAD trial activation service

This service prevents the one-day free trial from resetting just because the Android app is uninstalled or its local data is cleared.

## Security model

- The Android app sends its privacy-preserving MusaCAD device/license ID over HTTPS.
- D1 stores the first trial start and fixed expiry for that device ID.
- The service signs `MT1|<deviceId>|<expiresAtMs>` with RSA/SHA-256.
- The release build receives the matching trial public key through `MUSACAD_TRIAL_PUBLIC_KEY_PEM` and verifies the signed token locally.
- Reinstalling the app with the same signing key normally produces the same device ID on Android 8+; the service therefore returns the original expiry while it is active and `used` after it expires.
- The private trial-signing key must exist only as a server secret. It must be different from both the paid-license private key and the Android APK signing key. Never commit it, place it in `wrangler.toml`, or package it in the APK.

## Deploy outline

1. Create a Cloudflare Worker and D1 database.
2. Copy `wrangler.toml.example` to a local `wrangler.toml` and fill in the D1 database ID. Do not commit the local file.
3. Apply `schema.sql` to the D1 database.
4. Generate a dedicated RSA keypair for trial signing. Add its PKCS#8 private key as the encrypted Worker secret `MUSACAD_TRIAL_PRIVATE_KEY_PEM`.
5. When building MusaCAD, set the matching public key PEM as `MUSACAD_TRIAL_PUBLIC_KEY_PEM`. Do not reuse `MUSACAD-LICENSE-PUBLIC.pem`; that key is reserved for paid licenses.
6. Deploy the Worker and set the final HTTPS endpoint ending in `/v1/trial/start` as `MUSACAD_TRIAL_API_URL`.

The Android app refuses a non-HTTPS trial endpoint and does not follow trial-server redirects.

## API

`POST /v1/trial/start`

Request example:

```json
{
  "deviceId": "MC-12345678-90ABCDEF-12345678",
  "packageName": "com.musa.cad",
  "versionName": "1.0",
  "versionCode": 10
}
```

First request or request during the original 24-hour window:

```json
{
  "status": "active",
  "token": "MT1....",
  "expiresAtMs": 1780000000000
}
```

After the original window has expired:

```json
{
  "status": "used"
}
```

## Play Integrity

The base service deliberately does not require Play Integrity, so a properly signed direct-distribution APK can still offer the trial. If MusaCAD is distributed through Google Play, Play Integrity can be added as an additional server-side signal. Device recall is especially useful for reinstall abuse prevention, but it requires Play/Cloud setup and should be enabled only after the production Play app is configured.

## Production notes

- Put the Worker behind normal provider rate limiting / abuse controls before public sale.
- Keep three independent keys: Android release signing, paid-license signing, and trial signing.
- Back up the paid-license and trial private keys securely, separately. Losing either one breaks future issuance for its token domain.
- Never rotate either public verification key without a migration plan for already-issued tokens.
