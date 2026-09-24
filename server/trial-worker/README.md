# MusaCAD license backend

This Cloudflare Worker provides two security services for MusaCAD Android:

- one-time 24-hour trial activation that survives uninstall/reinstall;
- server-side Google Play purchase verification before MusaCAD Pro is granted.

## Security model

### Trial

- The Android app sends its privacy-preserving MusaCAD device/license ID over HTTPS.
- D1 stores the first trial start and fixed expiry for that device ID.
- The service signs `MT1|<deviceId>|<expiresAtMs>` with RSA/SHA-256.
- Reinstalling the app with the same signing key normally produces the same app-scoped device ID on Android 8+, so the original expiry is retained.
- After the original 24-hour window, the server returns `used`; reinstalling does not create another trial.

### Google Play

- MusaCAD attaches its privacy-preserving device/license ID with `setObfuscatedAccountId()` when the Play purchase flow starts.
- The app sends the Play `purchaseToken` to `/v1/play/verify`; the app never grants Pro solely from the client-side purchase callback.
- The Worker obtains an Android Publisher OAuth token with a dedicated Google service account.
- The Worker calls `purchases.productsv2.getproductpurchasev2` and grants only `PURCHASED` state for the configured MusaCAD Pro product.
- `PENDING`, cancelled, wrong-product, wrong-device and replayed purchase tokens do not grant Pro.
- D1 stores a SHA-256 hash of each purchase token as a unique key, not the raw purchase token.
- The Worker acknowledges a verified non-consumable purchase with `purchases.products.acknowledge`.
- By default, purchases without a MusaCAD device binding are rejected. Set `MUSACAD_PLAY_ALLOW_LEGACY_UNBOUND=true` only for an intentional migration of older purchases.

## Deploy outline

1. Create or reuse the Cloudflare Worker and D1 database.
2. Copy `wrangler.toml.example` to a local `wrangler.toml`; fill in the D1 database ID and non-secret variables.
3. Apply `schema.sql` to D1. It creates both the trial table and the verified Play purchase table.
4. Generate a dedicated RSA keypair for trial signing and store the PKCS#8 private key as the encrypted Worker secret `MUSACAD_TRIAL_PRIVATE_KEY_PEM`.
5. In Google Cloud / Play Console, create a service account authorized to use the Google Play Android Publisher API for the MusaCAD app.
6. Put the service account email in `MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL`.
7. Store the service account PKCS#8 private key only as the encrypted Worker secret `MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM`.
8. Set `MUSACAD_PACKAGE_NAME=com.musa.cad` and `MUSACAD_PLAY_PRODUCT_ID=musacad_pro`.
9. Deploy the Worker.
10. Build the Android release with:
   - `MUSACAD_TRIAL_API_URL=https://<worker>/v1/trial/start`
   - `MUSACAD_TRIAL_PUBLIC_KEY_PEM=<matching trial public key>`
   - `MUSACAD_PLAY_VERIFY_URL=https://<worker>/v1/play/verify`
   - `MUSACAD_PLAY_PRODUCT_ID=musacad_pro`

The Android app refuses non-HTTPS trial and Play verification endpoints.

## APIs

### `POST /v1/trial/start`

Request:

```json
{
  "deviceId": "MC-12345678-90ABCDEF-12345678",
  "packageName": "com.musa.cad",
  "versionName": "1.2.0",
  "versionCode": 15
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
{ "status": "used" }
```

### `POST /v1/play/verify`

Request:

```json
{
  "deviceId": "MC-12345678-90ABCDEF-12345678",
  "packageName": "com.musa.cad",
  "productId": "musacad_pro",
  "purchaseToken": "<Google Play purchase token>",
  "versionName": "1.2.0",
  "versionCode": 15
}
```

Verified and acknowledged purchase:

```json
{
  "status": "active",
  "productId": "musacad_pro",
  "acknowledged": true
}
```

Pending payment:

```json
{ "status": "pending" }
```

Invalid, cancelled, wrong-device or replayed purchase:

```json
{ "status": "denied" }
```

## Google Play Console prerequisites

- Create the one-time, non-consumable product ID `musacad_pro`.
- Upload a release to an internal testing track before purchase testing.
- Add license testers / internal testers as needed.
- Authorize the backend service account for the MusaCAD Play Console app and Android Publisher API.
- Keep the Android app signing key, offline paid-license private key, trial-signing private key, and Google service-account private key separate.

## Play Integrity

Play Integrity can be added later as another backend signal. The current design already keeps purchase verification and acknowledgement off the device. Play Integrity should be enabled only after the production Play app and Cloud project are configured.

## Production notes

- Put the Worker behind normal provider rate limiting / abuse controls.
- Never commit any private key or service-account JSON.
- Back up the trial and offline paid-license private keys securely and separately.
- Keep the Android release signing key stable; changing it can change the app-scoped Android ID used by MusaCAD licensing.
- For refunds and revocations, add Google Play RTDN/Voided Purchases synchronization before large-scale public sales.
