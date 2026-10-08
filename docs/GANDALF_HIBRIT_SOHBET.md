# MusaCAD Gandalf — sohbet odaklı hibrit AI

## Temel deneyim
Kullanıcı teknik komut kodları veya ayrı uzman düğmelerini bilmek zorunda değildir.
Gandalf alt panelinde bir sohbet alanı, **Ses**, **Gönder** ve üstte **PDF olarak
görüntüle** vardır. PDF seçeneği, açık sohbet penceresindeki **son Gandalf
yanıtını** PDF'ye dönüştürür ve Android PDF görüntüleyicisini açar.
PDF görüntüleme uygulaması yoksa paylaşım ekranı kullanılır.

Örnek eşdeğer istekler:
- “Metraj çıkar”, “Bana metrajını göster”, “Çizimin malzeme miktarlarını bul”
- “Keşif hazırla”, “Projeden keşif oluştur”, “İmalat listesi oluştur”
- “Bu projede eksik var mı?”, “Çizimde bir sorun var mı?”
- “Raporu PDF olarak görüntüle”

## Çalışma düzeni
1. **Yerel CAD analizi:** DWG nesne kimliği, metraj, vektör katmanı, çizim
   birimi, mühendislik uzman denetimi, ölçü ve onaylı işlem yürütücüsü
   telefonda çalışır.
2. **Yerel niyet normalizasyonu:** Sadece emin olunan *güvenli* yüksek seviyeli
   komutlar eşdeğer bir isteğe dönüştürülür. Sil/taşı/kopyala/çizim değiştirme
   gibi riskli eylemler yaklaşık kelime eşleştirmesiyle uygulanmaz.
3. **Bulut AI yorumlama:** Yerel araçların kesin bir yanıt üretemediği açık
   uçlu sorular, kullanıcı önceden izin vermişse MusaCAD'in HTTPS bulut AI
   ağ geçidine gönderilir; izin yoksa önce izin sorulur.
4. **Yerel destek bilgisi:** Kısıtlı çizim CAD-JSON'u yanında cihazdaki
   mühendislik motorunun **ön** metraj/nesne tespitlerinin kısa özeti
   gönderilebilir. Bunlar fiyat listesi, resmî poz onayı veya imalat
   kabul belgesi değildir.
5. **Takip soruları:** Aynı panel içindeki son dört kısa soru-yanıtın en fazla
   2600 karakteri konuşma bağlamı olarak gönderilir. Panel kapandığında
   bu bağlam kalıcı tutulmaz.
6. **Görsel ve çoklu proje:** Aynı bağlam, izinli görsel analiz gruplarında
   ve çoklu proje modunda da kullanılabilir.
7. **Bağlantı sorunu:** Sunucu yoksa veya isteğin zaman aşımı olursa bulut
   bağlantısı başarılıymış gibi gösterilmez; yerel CAD denetimi açıkça
   “yerel yedek analiz” olarak sunulur.

## Gizlilik, maliyet ve sınırlar
- İzin öncesi proje verileri, yerel bulgular ve konuşma bağlamı buluta
  gönderilmez. Çevrim dışı/yerel tercihine uyan istekler ağ çağrısı yapmaz.
- Bulut AI'nın çalışabilmesi için derlemede
  `MUSACAD_AI_API_URL` ve `MUSACAD_AI_SESSION_URL` HTTPS adresleri,
  sunucuda model sağlayıcı anahtarları ve geçerli lisans/oturum gerekir.
  Aygıtta model sağlayıcı API anahtarı tutulmaz.
- GitHub kalite/test derlemeleri AI servislerinin 7/24 çalıştığını veya
  canlı yanıt verdiğini göstermez.
- Otomatik araç önerileri, kullanıcının açık **önizleme ve uygulama onayı**
  olmadan çizime işlenmez.
- PDF düğmesi **yanıtı/raporu PDF'ye dönüştürür**; DWG çiziminin tüm paftalarını
  otomatik PDF'ye basma işlemi farklı bir yazdırma akışıdır.
- Benzer sorularda aynı bilgi ve yöntem kullanımı hedeflenir; model yanıtları
  kelime kelime veya sonuç sayısı açısından matematiksel olarak deterministik
  değildir. Gerçek mühendislik miktarları kaynakla doğrulanır.

## Testler
- `MusaAiConversationalIntentTest`: farklı söyleyişlerde eşdeğer niyet,
  yerel/çevrimdışı komutlar ve edit güvenliği.
- `MusaAiPanelContractTest`: sohbet odaklı panel, PDF düğmesi, kısa hafıza.
- `MusaAiCloudIntegrationContractTest`: güvenli bulut opt-in / ağ geçidi.
- `server/ai-worker/test.mjs`: model isteğine yerel/kısa sohbet bağlamı dahil edilmesi.
- Android lint / unit / compile ve gerçek telefon testi yapılmalıdır.
