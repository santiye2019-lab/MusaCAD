# MusaCAD production setup

Bu akış production secret'larını kaynak koda koymadan hazırlamak ve doğrulamak içindir.

## 1. Yerel signing/trial materyali

Windows/Android Studio bilgisayarında PowerShell aç:

```powershell
powershell -ExecutionPolicy Bypass -File tools/production/setup-production-secrets.ps1
```

Script:
- MusaCAD ana APK için kalıcı RSA Android keystore üretir.
- License Manager için ayrı kalıcı Android keystore üretir.
- Bilgisayarda OpenSSL varsa trial sunucusu için ayrı RSA private/public keypair üretir.
- JKS dosyalarını Base64'a çevirip GitHub Secret adlarını gösteren yerel bir template hazırlar.
- Parolaları template dosyasına yazmaz.
- Var olan release keystore dosyasının üzerine yazmaz.

**Release signing key'i kaybedilmemelidir.** En az iki şifreli offline yedek oluştur.

## 2. Cloudflare Worker / D1

Repo içindeki:
- `server/trial-worker/wrangler.toml.example`
- `server/trial-worker/schema.sql`

kullanılır.

Production Worker'da secret olarak:
- `MUSACAD_TRIAL_PRIVATE_KEY_PEM`
- `MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM`

tanımlanır.

Variable olarak:
- `MUSACAD_PACKAGE_NAME=com.musa.cad`
- `MUSACAD_PLAY_YEARLY_PRODUCT_ID=musacad_yearly_renewal`
- `MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL=<service-account-email>`
- `MUSACAD_PLAY_ALLOW_LEGACY_UNBOUND=false`

D1 şeması production database'e uygulanır.

## 3. GitHub Actions secrets

Repository > Settings > Secrets and variables > Actions:

Secrets:
- `MUSACAD_RELEASE_KEYSTORE_B64`
- `MUSACAD_KEYSTORE_PASSWORD`
- `MUSACAD_KEY_ALIAS`
- `MUSACAD_KEY_PASSWORD`
- `MUSACAD_TRIAL_API_URL`
- `MUSACAD_TRIAL_PUBLIC_KEY_PEM`
- `MUSACAD_PLAY_VERIFY_URL` (Play dağıtımı için)
- `MUSACAD_MANAGER_KEYSTORE_B64`
- `MUSACAD_MANAGER_KEYSTORE_PASSWORD`
- `MUSACAD_MANAGER_KEY_ALIAS`
- `MUSACAD_MANAGER_KEY_PASSWORD`

Repository variable:
- `MUSACAD_PLAY_YEARLY_PRODUCT_ID=musacad_yearly_renewal`

## 4. Production preflight

GitHub Actions > **Production release preflight** workflow'unu manuel çalıştır.

Bu workflow:
- secret'ların mevcut olduğunu,
- signing JKS alias/parolalarının doğru olduğunu,
- trial public key'in parse edildiğini,
- production HTTPS endpoint'lerinin erişilebilir olduğunu,
- Gradle'ın production release güvenlik kapılarının geçtiğini

doğrular.

**Bu workflow APK/AAB üretmez.**

## 5. Gerçek telefon testi

`DEVICE_FINAL_QA.md` maddeleri gerçek Android cihazında uygulanır. Production preflight başarılı olsa bile bu kontrol yapılmadan final release onayı verilmez.

## Güvenlik

- JKS/private key/parolaları sohbete, issue'ya, commit'e veya README'ye yazma.
- Trial private key ile MC1 lisans private key aynı değildir.
- APK signing key, License Manager signing key, MC1 private key, trial private key ve Google service-account private key ayrı tutulur.
