# MusaCAD Qwen3.5-4B — Çevrim içi görsel ve sayısal DWG AI çekirdeği

## Çalışma modeli

MusaCAD (Android) → mevcut lisans imzalı `MAI1/MAI2` oturumu
→ Cloudflare `musacad-ai` Worker
→ operatörün HTTPS ve **bearer erişim denetimli** model geçidi
→ Ollama / başka OpenAI-uyumlu multimodal model (`qwen3.5:4b`).

DWG sayısal vektör verisi MusaCAD tarafından hazırlanır. Kullanıcı onaylı
görsel kırpımlar (JPEG) ve bu CAD verisi aynı soruda Qwen'e gönderilir.
Qwen ham DWG/DXF dosyasını kendi başına ayrıştırmaz. Modelin vardığı teknik
sonuç, belge/çizim kanıtıyla ilişkilendirilmedikçe doğrulanmış sayılmaz.
Çizim değiştiren AI araçları sadece **öneri** üretir ve Android'de ayrıca
kullanıcı onayı gerektirir.

## Sunucu hazırlığı (örnek: operatöre ait Linux sunucu)

1. Güvenilen sunucuya Ollama kurun; resmi belgelere göre çalıştırın.
2. `ollama pull qwen3.5:4b`
3. Yerelde önce `http://127.0.0.1:11434/v1/chat/completions` adresine
   basit bir test göndererek metin yanıtı ve görsel destek olduğuna bakın.
4. Ollama portunu internete açmayın. TLS/HTTPS bitiren, **bearer doğrulayan**
   bir ters vekil veya özel model geçidi ve erişim filtresi kurun.
   Geçit yalnız `/v1/chat/completions` isteğini Ollama'ya yönlendirsin;
   diğer servis uç noktaları internete açılmasın.
5. İnternetten yalnız geçidin doğrulanmış
   `https://model.example.com/v1/chat/completions` adresi erişilebilir
   olsun; OAuth/servis anahtarı, zaman aşımı ve bağlantı sayısı sınırları
   sunucu tarafında korunsun.
6. GitHub MusaCAD → Settings → Secrets and variables → Actions:
   `MUSACAD_SELFHOSTED_AI_ENDPOINT` ve
   `MUSACAD_SELFHOSTED_AI_API_KEY` adlı iki **repository secret**
   oluşturun. Gerçek anahtarları sohbete, koda veya README'ye yapıştırmayın.
   İsteğe bağlı `MUSACAD_SELFHOSTED_MODEL` repository variable
   tanımlayın; varsayılan `qwen3.5:4b`.
7. `Actions → Deploy Gandalf AI and build APK → Run workflow`
   menüsünden `provider=selfhosted` seçin. Ön test, sunucu yanıt vermiyorsa
   Cloudflare'ı değiştirmeden hata verir. `provider=gemini` mevcut
   yedek/varsayılan yoldur; sağlayıcı geçişi kendiliğinden yapılmaz.

## Kabul testleri (gerçek telefonda)

- Basit "Merhaba" isteği başarılı, raporda
  `Çevrim içi AI motoru: Özel sunucu • qwen3.5:4b` görünüyor.
- DWG vektör JSON kaynak ID ve onaylı JPEG bölge, Qwen'e birlikte iletilir.
- Türkçe "Pis su bağlantısını analiz et" komutu, çizimde gördüğü
  ile göremediği ayrıntıları açıkça ayırır.
- 3×3 bölge taraması *tam proje/pafta analizi değildir*.
  Henüz incelenmeyen paftaların tümü açıkça listelenir.
- Dış modelde hata, boş yanıt, 429 hız/kaynak sınırı veya zaman aşımı
  yaşanırsa yerel DWG analizi korunur, tamamlanmayan görsel kontroller
  tamamlandı diye raporlanmaz.
- Bir talep çizim değişikliği içeriyorsa araç çıktısı öneri olarak kalır;
  kalıcı değişiklik yalnız kullanıcı onayıyla yapılabilir.
- Büyük pafta için 65 saniyelik tek istek sunucu sınırı, telefonda ise
  90 saniyelik ağ sınırı devam eder. Sunucu yeterince hızlı değilse
  4B modelle bile zaman aşımı olabilir. Kademeli tarama ve kaldığı yerden
  devam etme ayrı geliştirme gerektirir.

## Fiyat ve sınırlar

Qwen3.5 modelinin kendisi açık ağırlıklıdır; kullanımda Gemini API kotası
olmaz. Kendi bilgisayarınız elektrik ve bakım maliyetiyle, kiralık bir
GPU sunucu ise aylık/saatlik ücretle çalışır. Sistem kaynaklarında hız,
kullanıcı ve eşzamanlı istek sınırları bulunur; **sonsuz ücretsiz hizmet
vaat edilemez**.

Bu modül ilk aşamada ayrı GitHub dalında test edilir. Mevcut üretim
Cloudflare Worker, lisans Worker ve APK, operatörün gerçek özel sunucusu
hazır olmadan otomatik olarak değiştirilmez.
