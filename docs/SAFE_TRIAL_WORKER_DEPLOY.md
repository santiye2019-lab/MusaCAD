# MusaCAD — lisans/trial Worker güvenli kod güncellemesi

## Sorun

Cloudflare panosunda `musacad-trial-api` için `MUSACAD_DEVELOPER_DEVICE_IDS`
Production değişkeni eklenmiş olsa da eski dağıtımın kaynak kodu bu değişkeni
kullanmıyor olabilir. Bu nedenle `Active cloud-AI entitlement not found` (HTTP
403) hatası devam edebilir. Cloudflare'ın 100% deployment göstergesi yeni kaynak
kodunun doğru olduğunu tek başına kanıtlamaz.

Eski `deploy-gandalf-ai.yml` akışı, yalnız oturum adresi 404 dönerse lisans
Worker'ını güncelliyordu. Oysa yanlış/eski kod da 403 döndürebilir. Lisans
servisi, APK derlemesinin yan etkisi olarak artık **değiştirilmez**.

## Güvenli güncelleme prosedürü

1. GitHub → `santiye2019-lab/MusaCAD` → **Actions** → **MusaCAD License
   Worker — guarded production repair** → **Run workflow**.
2. İlk çalıştırmada **operation: inspect** seç. Confirmation boş kalabilir.
   Bu aşamada Production'a hiçbir kod gönderilmez.
3. İşlem, GitHub'daki trial Worker Node testlerini ve guard testlerini çalıştırır,
   Cloudflare lisans Worker'ının mevcut bağlarını **okur**, D1 veritabanı,
   `MUSACAD_DEVELOPER_DEVICE_IDS`, uygulama paket adı ve özel imza anahtarı
   binding'ini kontrol eder. Var olan D1 ID'si kullanılarak geçici Wrangler
   konfigürasyonu oluşturulur. `wrangler deploy --dry-run` ile yalnız derleme
   yapılır.
4. **Inspect başarılı** olursa ve kullanıcı sunucunun güncellenmesini açıkça
   onaylarsa yeniden **Run workflow**:
   - operation: `deploy`
   - confirmation: `DEPLOY_MUSACAD_TRIAL`
5. Dağıtımda `--keep-vars` açık kalır. Cloudflare'da tanımlanmış düz metin
   değişkenler korunur; Worker gizli anahtarları Deployment sırasında
   kaldırılmaz. D1 veritabanı oluşturma/silme/migration komutu çalışmaz.
6. Dağıtımdan sonra mevcut Worker bağları önceki snapshot ile karşılaştırılır.
   Başarısızsa GitHub Action hata verir ve manuel inceleme gerekir.
   Ayrıca **sahte** bir lisans kimliğiyle `/v1/ai/session` test edilir:
   lisanssız kimliğe HTTP 403 dönmelidir. Gerçek cihaz kimliği asla GitHub
   Actions log'una konmaz.

### Bilerek engellenen durumlar

- GitHub secret'ındaki `MUSACAD_TRIAL_API_URL` hedefi tam olarak
  `https://musacad-trial-api.musacad2019.workers.dev/v1/trial/start`
  adresi değilse **deploy reddedilir**.
- D1 `DB` bağının ID'si bulunamazsa, ek D1 varsa, beklenmeyen başka binding
  türü varsa, özel anahtar binding'i yoksa, geliştirici izin listesi boş veya
  biçimi hatalıysa dağıtım **başlamaz**.
- Build/deploy kayıtlarında API token, cihaz kimliği, özel anahtar veya bütün
  Cloudflare ayar JSON'u **yazdırılmaz**.
- Anahtarı veya cihaz kimliğini yeni kodun içine eklemek **yasaktır**.

### Kontrolün sınırları

Guard ancak tanımlanmış varsayımlarla mevcut D1 ve binding'leri doğrular.
Cloudflare'daki geriye dönük sürüm/rollback ve servis sahibi onayı, kritik
Production güncellemeleri için ayrıca önemlidir. `--keep-vars` dashboard
değişkenlerini korur ancak yeni Cloudflare binding türleri eklenirse guard
bilerek durur.

Dağıtımdan sonra telefonda MusaCAD'i tamamen kapatıp yeniden açarak
Gandalf'a kısa bir soru gönderin. Yeşil **AI bağlı** yalnızca gerçek model
cevabından sonra görünür. Bağlantı hâlâ reddediliyorsa Android uygulamasının
oturum URL'sinin doğru Worker'ı işaret ettiği, cihaz kimliğinin gerçekten
aynı olduğu ve imzalı oturum yanıtı ayrıca incelenmelidir.

**Önemli:** Gerçek kullanıcı kimliği veya gizli sunucu değerlerini GitHub
issue/commit, ChatGPT mesajı veya ekran görüntülerinde paylaşmayın.
