# Gandalf görsel bağlantı kopması — teşhis protokolü (11.10.2026)

## Saha bulgusu
Telefon, görsel DWG incelemesinde `SocketException` yakaladığında sunucu/ağ bağlantısının sıfırlandığını bildiriyor. Bunun **tek başına** sebebi görüntü boyutu, internet kesintisi veya Qwen kotası olarak belirlenemez. Başarısız grup görsel olarak incelenmiş sayılmaz.

## Bu dalda önerilen değişiklikler
- Her HTTPS AI analiz çağrısına, kullanıcı / lisans / cihaz bilgisi içermeyen rastgele UUID ile `X-MusaCAD-Request-ID` eklenir.
- İstek 3 MiB güvenli istemci sınırını aşarsa daha ağa yüklenmeden HTTP 413 uyumlu yerel hata verilir. Var olan tek seferlik küçük görsel denemesi bu hata için kullanılabilir.
- Android hata metninde **aşama**, **gönderilecek paket KB** ve **izleme kodu** belirtilir.
- Worker yalnız geçerli yetkili **görsel** isteklerde izleme kodu, görüntü sayısı, base64 toplam karakter miktarı, HTTP upstream durumu ve geçen süreyi günlüğe yazar. **DWG, metin, görüntü, session token, cihaz ID, kullanıcı/konuşma içeriği loglanmaz.**
- Bulut AI tarafından gerçekten yanıtlanmayan pafta ve görsel bölgeleri uygunluk raporunda incelenmiş olarak işaretlenmez.

## Test sırası
1. GitHub Actions üzerinden bu dalın Node AI-worker testleri, Android derlemesi ve ilgili sözleşme testleri geçmeli.
2. Mevcut APK test cihazında değiştirilmeden; derlenmiş test APK'sı üzerinde önce yalnız `/health`, ardından küçük ve geçerli görsel tarama, sonra Silivrikapı gerçek DWG taraması denenmeli.
3. Ağ koparsa Android'deki izleme kodu Cloudflare Worker günlüğünde aranmalı. "ACCEPTED" yoksa istek Worker işleme hattına ulaşmamış/işlenmemiş olabilir; "ACCEPTED" var ama "UPSTREAM" yoksa timeout veya upstream bağlantı hatası kontrol edilmeli.
4. HTTP 413, 429, 502/503/504 ayrı yorumlanmalı; 200 `/health` modelin de çalıştığını **kanıtlamaz**.
5. İlk ve varsa küçültülmüş ikinci istek paketi KB ve süreleri karşılaştırılmalı. Görsel kapsama ve gerçek ekipman/çap okuması ayrıca kontrol edilmeli.

## Bilinen sınırlar
Bu sürüm **kök nedeni kanıtlayan veya kalıcı bağlantı onarımı sağlayan bir yama değildir**; hata anındaki kanıtı görünür kılar ve aşırı büyük gönderimleri azaltır. Çok katlı DWG'de ayrı kat / kesit / çatı / vaziyet paftalarının eksiksiz mühendislik denetimi, yalnız bu ağ değişikliğiyle sağlanmaz. Canlı Cloudflare sunucusuna dağıtım ve telefonda gerçek DWG kabul testi ayrıca gereklidir.
