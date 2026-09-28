# MusaCAD release signing

MusaCAD release builds external Android signing key kullanır. Private keystore repoya veya kaynak koda eklenmez.

## Ana uygulama imza değişkenleri

- `MUSACAD_KEYSTORE_FILE`
- `MUSACAD_KEYSTORE_PASSWORD`
- `MUSACAD_KEY_ALIAS`
- `MUSACAD_KEY_PASSWORD`

## Production lisans/trial değişkenleri

- `MUSACAD_TRIAL_API_URL` — HTTPS production trial endpoint
- `MUSACAD_TRIAL_PUBLIC_KEY_PEM` — trial token doğrulama public key'i
- Play dağıtımı için ayrıca:
  - `MUSACAD_PLAY_VERIFY_URL` — HTTPS server-side Play doğrulama endpoint'i
  - `MUSACAD_PLAY_YEARLY_PRODUCT_ID`

## Güvenli production build

Validation CI release derlemesi secret olmadan compile edebilir; bu ticari paket değildir.

Gerçek production direct release:

```bash
export MUSACAD_PRODUCTION_RELEASE=true
export MUSACAD_KEYSTORE_FILE=/secure/musacad-release.jks
export MUSACAD_KEYSTORE_PASSWORD='...'
export MUSACAD_KEY_ALIAS='...'
export MUSACAD_KEY_PASSWORD='...'
export MUSACAD_TRIAL_API_URL='https://...'
export MUSACAD_TRIAL_PUBLIC_KEY_PEM='-----BEGIN PUBLIC KEY-----...'
./gradlew clean assembleDirectRelease
```

Production flag açıkken Gradle şu durumlarda build'i durdurur:
- release signing secret'larından biri eksikse,
- keystore dosyası yoksa,
- trial endpoint HTTPS değilse,
- trial public key eksik/geçersiz görünüyorsa,
- offline paid-license public key asset eksikse,
- Play release için server verification endpoint veya product ID eksikse.

Ayrıca production build'de legacy 12 haneli kısa kod aktivasyonu kapatılır; ticari offline aktivasyon RSA imzalı MC1 ile yapılır.

## License Manager APK imzası

License Manager için ayrı APK signing secret'ları kullanılır:

- `MUSACAD_MANAGER_KEYSTORE_FILE`
- `MUSACAD_MANAGER_KEYSTORE_PASSWORD`
- `MUSACAD_MANAGER_KEY_ALIAS`
- `MUSACAD_MANAGER_KEY_PASSWORD`

Production manager build için `MUSACAD_MANAGER_PRODUCTION_RELEASE=true` kullanılır.

APK signing anahtarı ile **MC1 lisans RSA private key aynı anahtar değildir**. MC1 key yaşam döngüsü `LICENSE_MANAGER.md` dosyasında açıklanır.

Android/Play Protect, doğru imzalanmış sideload APK'da dahi kaynağa/cihaz politikasına bağlı uyarı gösterebilir; uygulama bu işletim sistemi uyarısını güvenli biçimde kaldıramaz.
