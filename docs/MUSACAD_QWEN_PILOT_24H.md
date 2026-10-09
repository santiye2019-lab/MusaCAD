# MusaCAD — izolasyonlu 24 saatlik Qwen pilotu

Bu prosedür, mevcut `musacad-ai` (Gemini) Worker'ını değiştirmeden
Cloudflare Workers AI `@cf/qwen/qwen3.8-27b` ile gerçek bir DWG üzerinde
sınırlı ve denetlenebilir saha testi yapabilmek içindir.

## Güvenlik ve kullanım sınırları

- Pilot Worker adı sabittir: `musacad-qwen-pilot`. Üretim Worker'ının adı
  `musacad-ai` olarak kalır.
- `POST /v1/analyze`, imzalı ve süresi geçmemiş **MAI2/developer**
  oturumu gerektirir; normal MAI1 oturumları reddedilir.
- Pilotun bitiş zamanı Worker'a dağıtımda değişken olarak yazılır.
  Her analiz isteğinde bu zaman denetlenir. Süresi dolan Worker
  yanıt vermeyi bırakır; sonradan fiziksel silme için `operation=remove`
  kullanılmalıdır.
- Gerçek DWG dosyası GitHub'a veya model uç noktasına bütün halinde
  aktarılmaz. MusaCAD yalnız kullanıcı izniyle sınırlı CAD-JSON ve
  JPEG çizim bölgeleri gönderir.
- Model, geçerli çizim kanıtıyla doğrulanamayan boru çapı, kot, keşif
  miktarı veya mevzuat uygunluğu sonucu **uydurmamalıdır**.
- Workers AI ücretsiz Neuron kotası vardır. **Kotasız hizmet değildir**;
  ücretli tüketim ve hız sınırı hesabın koşullarına bağlıdır.

## Çalıştırma

1. GitHub'da `Actions → Isolated Qwen 24-hour developer pilot → Run workflow`
   seçin.
2. `operation=deploy` ile başlatın. İsteğe bağlı
   `build_pilot_apk=true` seçimi, Qwen pilot uç noktasına bağlı,
   üretim sertifikasıyla imzalanmış **yalnızca test amaçlı** APK üretir.
3. Önce gerçek Cloudflare Qwen metin denemesi ve Worker testleri geçer.
   Başarısızsa pilot Worker'a dokunulmaz.
4. Pilot ayrı isimle dağıtılır, yalnız ona AI erişim anahtarı ve
   MusaCAD'ın AI-session imza doğrulama **açık** anahtarı yüklenir.
5. İş akışının `Summary` bölümünden pilot URL'si ve kesin UTC
   bitiş zamanı görüntülenir. İmzasız istek reddi kontrol edilir.
6. APK istenmişse ilgili Actions çalışmasının `Artifacts`
   bölümündeki `MusaCAD-Qwen-24h-PILOT-signed` dosyasını kullanın.
   **APK aynı uygulama paket kimliğini taşır; kurulum mevcut MusaCAD'ı
   değiştirebilir.** Yerel verileri yedekleyin ve pilot sonrasında
   kararlı sürümü yeniden yükleyin. APK kendiliğinden telefona kurulmaz.
7. Test sonunda `operation=remove` ile pilot Worker'ı silin.
   Süre aşımında analiz zaten reddedilir ancak Worker otomatik silinmez.

## Gerçek DWG kabul testi

Telefonda uygulamaya gerçek dosyayı açtıktan sonra:

1. Yapay zekâ bağlantısının `provider=cloudflare` ve
   `model=@cf/qwen/qwen3.8-27b` olduğunu doğrulayın. Yanıt alamadan
   yeşil bağlantı göstergesini tamamlanmış analiz saymayın.
2. “Projeyi pafta pafta analiz et; her bulgu için pafta, kanıt, cihaz
   etiketi/handle, doğrulama durumu ve düzeltme önerisini yaz” deyin.
3. Çizimin genel görünüşüne ek olarak yeterli yakınlaştırmalı bölge
   görüntüsünün **açık bulut izniyle** gönderildiğini kontrol edin.
4. Yazılan adet, etiket ve sourceId/handle eşleşmesini CAD cetveliyle
   karşılaştırın. Birim `$INSUNITS=0` olduğunda metre/metraj üretmeyin;
   önce çizim birimini bağımsız doğrulayın.
5. Kapsamı ölçün: açılan pafta sayısı, görsel olarak incelenen bölge
   sayısı, atlanan paftalar, rapor edilen teknik bulgular.
6. Süre, `429`/timeout durumu, Neuron tüketimi ve kullanıcı açısından
   kabul edilebilir yanıt hızı ayrıca raporlansın.

Mevcut görsel inceleme yöntemi tek DWG içinde 3×3 tarama ve başlık
yakın planları kullanabilir. Bu **tüm kat, kesit, çatı, vaziyet
paftalarının tam doğrulaması değildir**. Daha kapsamlı işleme için
pafta sınırı envanteri ve kaldığı yerden devam edebilen batch
tarama ayrıca geliştirilecektir.

## Bilinen ön koşul

Telefonun AI oturum servisinden gerçekten **MAI2/developer** imzalı
token alabilmesi gerekir. `Active cloud-AI entitlement not found`
hatası olan cihaz bu pilotla da analiz yapamaz; sorun lisans
servisindeki cihaz tanımında çözülmelidir. Cihaz kimliğini veya
Cloudflare gizli anahtarlarını sohbet/GitHub dosyalarına yazmayın.

**Durum:** Kod hazırlığı ve GitHub CI; gerçek telefon saha testi,
Worker dağıtımı veya gerçek DWG'den canlı Qwen raporu henüz
tamamlanmış sayılmaz.
