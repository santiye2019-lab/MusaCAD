# MusaCAD / Raporlar — Otomatik Tanılama

**Gizlilik:** `santiye2019-lab/MusaCAD` herkese açıktır. Buradaki `Raporlar/` klasörü sadece mimari/ayar belgeleri tutar. **Gerçek telefon raporları buraya yazılmamalıdır.**

## Kodda hazırlanan akış

1. Her DWG/DXF açma denemesi, sadece izinli sayısal ve durum alanlarından oluşan bir JSON raporunu uygulamanın özel `reports-pending/` dizinine otomatik kaydeder.
2. Uygulamanın **Otomatik Test Raporları** menüsünde kullanıcının açık izni olmadan hiçbir rapor telefon dışına gönderilmez. İzin kapatılabilir ve bekleyen yerel kayıtlar silinebilir.
3. İzin ve HTTPS adresi varsa uygulama kısa ömürlü imzalı MusaCAD oturumunu kullanarak `POST /v1/diagnostics` çağrısı yapar. Başarısız kayıtlar yerelde kalır, yeniden açılışta veya sonraki raporda tekrar denenir.
4. Cloudflare Worker, oturumu doğrular, JSON alanlarını katı biçimde sınırlar, GitHub API'den deponun **private=true** olduğunu doğrular ve yalnız bundan sonra şu konuma yazar: **özel depo** `Raporlar/YYYY-MM-DD/<anonim-cihaz-etiketi>/<rapor-kimliği>.json`.
5. Worker başarıyı bildirmeden uygulama yerel raporu silmez.

## Alanlar

`schema`, `id`, `createdAtMs`, `appVersion`, `format` (örn. AC1024), `event`, `fileBytes`, `entityCount`, `trace` (yalnız açılma aşaması/süre/heap sayıları), `errorKind` (sınıflandırılmış hata türü).

**Gönderilmeyenler:** gerçek DWG/DXF baytları, dosya adı/yolu/URI, çizim metinleri, koordinatlar, kişi adı, ekran görüntüsü, GitHub yetki anahtarı. Bu raporlar AutoCAD görsel eşdeğerliğini tek başına ispatlamaz.

## Yayına alma gereksinimleri — henüz yapılmadı

- Yeni bir **özel GitHub deposu** oluşturulmalı ve Worker'a yalnızca o depoda Contents read/write yetkisi verilmelidir; MusaCAD public deposuna yetki verilmemelidir.
- Worker secret: `MUSACAD_REPORTS_GITHUB_TOKEN`; Worker env: `MUSACAD_REPORTS_GITHUB_REPO=OWNER/PRIVATE_REPO`.
- Worker'da mevcut `MUSACAD_AI_SESSION_PUBLIC_KEY_PEM` doğrulama anahtarı olmalı ve değişiklikler güvenli biçimde yayına alınmalıdır.
- APK üretiminde `MUSACAD_REPORTS_API_URL=https://<worker-alan-adı>/v1/diagnostics` ayarlanmalıdır.
- Üretimde trafik limiti, yetki süresi, saklama/silme süresi ve sunucu güvenlik testi tamamlanmalıdır.
- GitHub'a otomatik rapor aktarımı, özel depo + sunucu yapılandırması ve yayımlanmış APK **olmadan fiilen etkin değildir**.

## Durum (03.11.2026)

Uygulama yerel raporlayıcı/izin ekranı, opt-in gönderim kuyruğu ve Worker'daki özel repo korumalı uç nokta geliştirme dalındadır. CI ve nihai testlerden sonra birleştirilmelidir. Sunucu yayımlanmadı; özel depo henüz bağlanmadı.
