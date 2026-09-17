# MusaCAD trial activation service

This service prevents the one-day free trial from resetting just because the Android app is uninstalled or its local data is cleared.

## Security model

- The Android app sends its privacy-preserving MusaCAD device/license ID over HTTPS.
- D1 stores the first trial start and fixed expiry for that device ID.
- The service signs `MC1|<deviceId>|<expiresAtMs>` with RSA/SHA-256.
- The APK contains only the matching public key and verifies the signed token locally.
- Reinstalling the app with the same signing key normally produces the same device ID on Android 8+; the service therefore returns the original expiry while it is active and `used` after it expires.
- The private license-signing key must exist only as a server secret. Never commit it, place it in `wrangler.toml`, or package it in the APK.

## Deploy outline

1. Create a Cloudflare Worker and D1 database.
2. Copy `wrangler.toml.example` to a local `wrangler.toml` and fill in the D1 database ID. Do not commit the local file.
3. Apply `schema.sql` to the D1 database.
4. Add the PKCS#8 RSA private key matching `app/src/main/assets/MUSACAD-LICENSE-PUBLIC.pem` as the encrypted Worker secret `MUSACAD_LICENSE_PRIVATE_KEY_PEM`.
5. Deploy the Worker and use the final HTTPS endpoint ending in `/v1/trial/start` as `MUSACAD_TRIAL_API_URL` when building MusaCAD.

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
  "token": "MC1....",
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
- Keep the Android release-signing key and license-signing private key separate.
- Back up the license-signing private key securely. Losing it prevents issuing licenses compatible with the embedded public key.
- Never rotate the embedded public key without a migration plan for existing licenses.
