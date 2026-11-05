# MusaCAD — Raporlar

Bu klasör MusaCAD'in gerçek DWG açma ve görüntüleme testlerinden üretilen tanılama raporları için planlanan GitHub kayıt noktasıdır. Kimlik, koordinat veya dosya adı içermeyen **toplu mühendislik kanıtı** ayrıca burada belgelenebilir.

**ÖNEMLİ: Bu MusaCAD deposu PUBLIC (herkese açık).** Bu nedenle kullanıcı cihazından toplanan gerçek raporlar **buraya yüklenmemelidir**. Bu klasör raporlama şeması, entegrasyon dokümantasyonu ve yalnızca toplu sayıları içeren [AC1024 LINE doğrulama özetini](AC1024_LINE_DOGRULAMA_OZETI_2026-11-05.md) içerir. İzin ve güvenli özel depo bağlantısı sağlanmadan hiçbir otomatik rapor yüklemesi aktif edilmemelidir.

## Test APK'sı ve kalıcı uygulama için hedef

- DWG açma denemesi, açma süresi, tespit edilen DWG sürümü, okuma aşaması, çizilen / atlanan nesne sayıları ve hata kodları yerel olarak raporlanır.
- Raporda varsayılan olarak DWG dosyasının kendisi, dosya yolu, dosya adı, kullanıcı bilgileri, çizim koordinatları, metin içeriği veya ekran görüntüsü bulunmaz.
- Ekran görüntüsü ve ayrıntılı çizim kanıtı yalnızca kullanıcı ayrıca açıkça seçerse eklenebilir.
- Kullanıcı onayıyla özel GitHub rapor deposuna gönderim yapılır. Gizli repo bilgileri ve erişim anahtarları APK içine gömülmez; aktarım için yetkilendirilmiş aracı servis gerekir.
- Her rapor işlem kimliği, zaman, uygulama/çekirdek sürümü, başarı durumu, hata aşaması ve güvenli toplam sayımlarla eşleştirilir. DWG açıldı bilgisi, görsel AutoCAD eşdeğerliği kanıtı değildir.
- Ağ hatasında rapor yerelde saklanır ve kullanıcı isterse yeniden gönderilir. Kullanıcı raporlamayı kapatabilir ve yerel raporları temizleyebilir.

## Durum

Klasör kuruldu. Rapor toplama, özel depoya aktarım, görüntü kanıtı ve otomatik karşılaştırma **henüz bağlı ya da tamamlanmış değildir**. Bunlar için ayrı kod, test, erişim yetkisi ve gizlilik doğrulaması gerekir.
