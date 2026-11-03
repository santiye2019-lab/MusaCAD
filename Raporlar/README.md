# MusaCAD — Otomatik DWG test raporları

**Geliştirme tercihi (3 Kasım 2026):** Bu herkese açık depoda, `Raporlar/` altında **yalnızca anonimleştirilmiş ve sınırlı teknik tanılama JSON'ları** tutulabilir. Proje dosyaları, adlar, çizim metinleri, yollar, koordinatlar, ekran görüntüleri, cihaz kimlikleri ve kullanıcı verileri **buraya yazılmaz**. Git geçmişindeki kayıtlar klasör silinse dahi kalabileceği için gerçek gizli veriler hiçbir aşamada gönderilmemelidir.

## Bağlantı

- Android: `MusaDiagnostics`, açılan DWG/DXF için aşama, süre ve sayım bilgilerini `musacad-diagnostics/v1` JSON olarak kaydeder; geçici ağ hatasında en fazla 24 raporu özel uygulama klasöründe tutar. Otomatik gönderim çizimin açılmasını engellemez.
- Android **debug/test** derlemesi varsayılan olarak otomatik raporlama açıktır; **release** derlemesi kullanıcı etkinleştirince açılır. Ana menüde **Tanılama raporları** üzerinden kapatılabilir ve sıradaki yerel raporlar silinebilir.
- Android, var olan `MUSACAD_AI_API_URL=https://<ai-worker>/v1/analyze` adresinden `/v1/diagnostics` yolunu türetir. Aktarım HTTPS ile, mevcut kısa ömürlü MusaCAD AI geliştirici oturumu üzerinden yapılır. APK içinde GitHub anahtarı yoktur.
- AI Worker: `POST /v1/diagnostics` imzalı **MAI2 developer** oturumu olmadan reddedilir; ekstra/bilinmeyen alanlar ve geçersiz veriler kabul edilmez. Worker, GitHub Contents API üzerinden `Raporlar/YYYY-MM-DD/<uuid>.json` yoluna yazar. Aynı raporun gönderilmesi tekrarlandıysa içerik doğrulanarak mükerrer kayıt önlenir.
- Cloudflare Worker'ın `MUSACAD_REPORT_GITHUB_TOKEN` isimli **server-side secret** değeri GitHub fine-grained token olmalıdır; yalnızca `santiye2019-lab/MusaCAD` deposunda **Contents: Read and write** erişimi gerekir. Secret eksikse uç nokta 503 döndürür ve telefon raporu saklar.

## Rapor alanları

`schema, reportId, occurredAt, appVersion, format, signature, result, stage, elapsedMs, inputBytes, parsedEntities, layerCount, conversionWarnings, errorCode`.

Bir `success` durumu yalnızca **uygulamanın ilgili açma yolunu sonlandırdığını** belirtir. Orijinal renkler, bloklar ve tüm 2B/3B geometri için AutoCAD ile eşdeğerlik anlamına **gelmez**.

## Devreye alma ve test

1. GitHub PR birleştirildikten ve ilgili Android + Worker testleri başarılı olduktan sonra Cloudflare Worker'a **açık dağıtım onayıyla** yeni kod dağıtılır. Kodun depoda bulunması canlı uç noktanın mevcut olduğu anlamına gelmez.
2. İşletim ortamında `wrangler secret put MUSACAD_REPORT_GITHUB_TOKEN` komutu ile yalnız sunucuya gerekli token verilir; token GitHub deposuna veya APK'ya yazılmaz.
3. Sahibin onayladığı test APK'sında gerçek bir DWG açılır; ağ varsa bir JSON GitHub'a düşer, ağ yoksa cihazda kuyrukta kalır. GitHub kaydı ve uygulama raporu **uçtan uca** karşılaştırılmadan canlı aktarım tamamlandı denmez.
4. Geliştirme sonunda otomatik debug raporlaması kapatılabilir; ancak geçmişi temizleme konusunda Git geçmişinin herkese açık olduğuna dikkat edilmelidir.

**Durum:** Bu dizindeki geliştirme rapor modu GitHub PR'da hazırlanmıştır; üretim Worker dağıtımı, token bağlantısı ve gerçek cihaz testi ayrıca doğrulanmalıdır.
