# MusaCAD production setup

Bu akış production secret'larını kaynak koda koymadan hazırlamak ve doğrulamak içindir.

## 1. Yerel signing/trial materyali

Windows/Android Studio bilgisayarında PowerShell aç:

```powershell
powershell -ExecutionPolicy Bypass -File tools/production/setup-production-secrets.ps1

# GitHub CLI ile signing secret'larını doğrudan yüklemek isterseniz:
powershell -ExecutionPolicy Bypass -File tools/production/setup-production-secrets.ps1 -SetGitHubSecrets
```

Script:
- MusaCAD ana APK için kalıcı RSA Android keystore üretir.
- License Manager için ayrı kalıcı Android keystore üretir.
- Bilgisayarda OpenSSL varsa trial sunucusu için ayrı RSA private/public keypair üretir.
- GitHub Secret adlarını içeren bir kontrol listesi hazırlar; Base64 JKS veya parola değerlerini dosyaya yazmaz.
- `-SetGitHubSecrets` seçeneğinde GitHub CLI üzerinden JKS Base64 ve parolaları bellekte doğrudan repository secrets'a aktarır.
- Var olan release keystore dosyasının üzerine yazmaz.

**Release signing key'i kaybedilmemelidir.** En az iki şifreli offline yedek oluştur.

## 2. Cloudflare Worker / D1

Repo içindeki:
- `server/trial-worker/wrangler.toml.example`
- `server/trial-worker/schema.sql`

kullanılır.

Production lisans Worker'ında secret olarak:
- `MUSACAD_TRIAL_PRIVATE_KEY_PEM`
- `MUSACAD_TRIAL_PUBLIC_KEY_PEM`
- `MUSACAD_AI_API_URL`
- `MUSACAD_AI_SESSION_URL`
- `MUSACAD_LICENSE_PUBLIC_KEY_PEM`
- `MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM`

tanımlanır.

Variable olarak:
- `MUSACAD_PACKAGE_NAME=com.musa.cad`
- `MUSACAD_PLAY_YEARLY_PRODUCT_ID=musacad_yearly_renewal`
- `MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL=<service-account-email>`
- `MUSACAD_PLAY_ALLOW_LEGACY_UNBOUND=false`

D1 şeması production database'e uygulanır.

Cloudflare hesabında D1 database'i oluşturduktan ve Google Play service-account bilgilerini hazırladıktan sonra:

```powershell
powershell -ExecutionPolicy Bypass -File tools/production/deploy-cloudflare.ps1 `
  -D1DatabaseId "<D1_DATABASE_ID>" `
  -PlayServiceAccountEmail "<service-account@project.iam.gserviceaccount.com>" `
  -PlayServiceAccountPrivateKeyPath "<google-private-key.pem>"
```

Bu script `wrangler d1 execute --remote` ile şemayı uygular, private key'leri `wrangler secret put` ile şifreli Worker secret olarak yükler ve Worker'ı deploy eder. Cloudflare CLI oturumu önceden açılmış olmalıdır.

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


## 6. Gandalf Cloud AI

Gandalf AI için ayrı `server/ai-worker` Worker'ı kullanılır.

AI Worker secrets:
- `OPENAI_API_KEY`
- `MUSACAD_AI_SESSION_PUBLIC_KEY_PEM`

AI Worker variables:
- `OPENAI_MODEL=<Responses API üzerinde desteklenen model>`
- `OPENAI_MAX_OUTPUT_TOKENS=3200`

Android production build ortamı:
- `MUSACAD_AI_SESSION_URL=https://<license-worker>/v1/ai/session`
- `MUSACAD_AI_API_URL=https://<ai-worker>/v1/analyze`

OpenAI API anahtarı hiçbir zaman APK'ya, GitHub repository dosyasına, issue'ya veya istemci BuildConfig alanına yazılmaz. Gandalf'ın yerel CAD araçları production APK'da Cloud AI olmadan da çalışır. Cloud AI etkinleştirilecekse `MUSACAD_AI_SESSION_URL` ve `MUSACAD_AI_API_URL` birlikte ve HTTPS olarak verilmelidir; tek endpoint verilirse production build durur.
