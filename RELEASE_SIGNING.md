# MusaCAD release signing

MusaCAD release builds support an external Android signing key. The private keystore must never be committed to this repository or embedded in source code.

Set these environment variables before building a commercial release:

- `MUSACAD_KEYSTORE_FILE` — absolute path to the private `.jks` / `.keystore`
- `MUSACAD_KEYSTORE_PASSWORD` — keystore password
- `MUSACAD_KEY_ALIAS` — signing-key alias
- `MUSACAD_KEY_PASSWORD` — signing-key password

Then build with:

```bash
./gradlew clean assembleRelease
```

When all four variables exist, Gradle signs the release with Android signature schemes v1-v4. Without them, CI can still compile an unsigned release for validation, but that unsigned artifact is not a commercial distribution package.

For public distribution, prefer Google Play App Signing or another managed signing process. Android/Play Protect may still show installation warnings for sideloaded APK files even when they are correctly signed; an app cannot legitimately disable those operating-system warnings.

Keep the production signing key and future MusaCAD license-signing private key separate. Only public verification material belongs inside the APK.
