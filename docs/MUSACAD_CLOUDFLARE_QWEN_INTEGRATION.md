# MusaCAD — Cloudflare Qwen3.8-27B ikinci AI motoru

## Bağlantı

Android uygulaması → mevcut `MusaAiSessionService` → imzalı MAI1/MAI2
oturumu → `musacad-ai` Cloudflare Worker → Cloudflare Workers AI
`@cf/qwen/qwen3.8-27b` → MusaCAD'ın rapor/öneri ekranı.

Böylece Android uygulamasına Cloudflare AI API anahtarı eklenmez.
Gandalf'ın `/v1/analyze` şeması ve lisans doğrulaması aynen korunur.
Bulut izni alınmış çizimden sınırlı `musacad-cad-json/v1` kaynak
bilgileri ve JPEG pafta kırpımları beraber gönderilir. Gerçek DWG dosyasının
tamamı bu uç noktaya gönderilmez.

## Hangi aşama tamamlandı?

- GitHub test #37967635681: Cloudflare Qwen Türkçe metin ve **sentetik**
  teknik çizim görselini bağımsız olarak doğru yanıtladı.
- Bu PR, `AI_PROVIDER=cloudflare` seçeneğini mevcut Gandalf Worker'a ekler.
- Her istek imzalı MusaCAD oturumu gerektirir, sağlayıcı adı raporda görünür,
  `reasoning_effort=low` ve `max_completion_tokens>=1536` ile
  görünür yanıt için yer ayrılır.
- Qwen HTTP 429 veya görünür yanıt vermezse analiz başarısız sayılır;
  429 için yanıltıcı "Gemini kotası doldu" mesajı gösterilmez.

## Manuel üretim aktivasyonu — hazır olunca

GitHub'da `Actions → Deploy Gandalf AI and build APK → Run workflow`
→ `provider=cloudflare` seçin. Bunu sadece saha ve kota testlerinden
sonra yapın. İşlem şu sırayla çalışır:

1. `CLOUDFLARE_ACCOUNT_ID` ve `CLOUDFLARE_AI_API_TOKEN` adlı
   GitHub Actions gizli değerlerini doğrular.
2. Önce mevcut Cloudflare Qwen REST API'ye düşük akıl yürütmeli
   ve sınırlı bir metin isteği gönderir; **yanıt yoksa üretime dokunmaz**.
3. Başarılıysa aynı `musacad-ai` Worker'ı `cloudflare` sağlayıcı
   moduyla dağıtır; anahtar yalnız Worker Secret olarak aktarılır.
4. APK üretir, fakat kullanıcı APK'yı kurana kadar telefon değişmez.

`provider=gemini` varsayılandır. Sadece GitHub kodunun ana dala
alınması canlı AI sağlayıcısını değiştirmez. Otomatik ücretli veya
başka bir API'ye sessiz geçiş yoktur.

## Kabul testleri

1. Türkçe doğal dil, pafta görseli, CAD kaynak kimliği ve disiplin
   kapsamı **aynı** Qwen isteğine girer.
2. Bütün kat/çatı/kesit/vaziyet görselleri tespit edilmeden "proje
   tamamen incelendi" sonucu yazılamaz.
3. Rapor: gözlenen görsel bulgu, DWG sourceId, pafta koordinatı ve
   belirsizliği ayrı gösterir; bağlanmayan nesne eşleştirilmiş sayılmaz.
4. Gerçek bir mekanik DWG ile telefon üzerinde saha denemesi,
   performans ve ücretsiz Neuron kullanım miktarı ölçülmeden
   Qwen varsayılan sağlayıcı yapılmaz.
5. Kullanıcı onayı olmayan çizim değişikliği uygulanamaz.

## Ücretsiz kullanım sınırı

Cloudflare Workers AI ücretsiz başlangıç kullanımına sahiptir; model
**kotasız değildir**. Qwen 3.8-27B için input ve output token'ları
Neuron tüketir. Büyük paftalarda çok sayıda görsel analiz isteği
kotayı hızlı tüketebilir; performans ve maliyet izlemesi gerekir.

Bu aşamada mevcut lisans Worker'ı, lisans veritabanı ve canlı Gemini
yapılandırması değiştirilmemiştir.
