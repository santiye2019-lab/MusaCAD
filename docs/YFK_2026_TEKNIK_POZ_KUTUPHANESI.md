# MusaCAD — 2026 ÇŞİDB / YFK teknik poz ve analiz kütüphanesi

## Amaç

MusaCAD'de **fiyatlardan ayrı**, dayanağı izlenebilir bir mekanik imalat sözlüğü:
poz kodu, pozla ilgili kaynak cilt/sayfa, basılı tarif ve analiz metinlerine
yerel erişim, proje metrajındaki malzeme adlarıyla **aday** arama.

**Bu repoda tam YFK kitapları, özgün tariflerin toplu kopyaları, fiyat cetveli
veya resmî analiz girdisi veri dökümü bulunmaz.** Bu dosya resmî kaynaklara
işaret eden bağlantıları ve cihazda kişisel kullanım için indeksleme
mekanizmasını tanımlar. Yayın hakları ve ticari yeniden dağıtım izni
ayrıca değerlendirilmelidir. Bir dosyanın ücretsiz görüntülenebilmesi
ticari uygulamaya toptan kopyalama lisansı vermez.

## Kaynaklar

2026 Ocak tarihli, ÇŞİDB Yüksek Fen Kurulu tarafından sunulan PDF'ler:
- [İnşaat ve Tesisat Birim Fiyatları](https://webdosya.csb.gov.tr/v2/yfk/2026/01/1-BF-202619011535-20260119155143.pdf) — mevcut `MusaAiYfkOfflineCatalog` ile ayrı özel arşiv.
- [Mekanik Analizler 1 — sıhhi/ısıtma](https://webdosya.csb.gov.tr/v2/yfk/2026/01/mekanik-analiz-1-sihhi-s-tma-tes-20260115162956.pdf)
- [Mekanik Analizler 2](https://webdosya.csb.gov.tr/v2/yfk/2026/01/mekanik-analiz-2-m-t-tes-20260115163006.pdf)
- [Mekanik Analizler 3](https://webdosya.csb.gov.tr/v2/yfk/2026/01/Mekanik-Tesisat-Birim-Fiyat-Analizleri-3-20260121121534.pdf)

Resmî liste: https://yfk.csb.gov.tr/birim-fiyatlar-100468

Kullanıcı tarafından eklenen `2026-Birim-Fiyat-Listesi-....pdf` adlı
68 sayfalık dosya **Döner Sermaye hizmet ücretleri** içerir; tesisat poz/analiz
kitabı değildir ve bu veri tabanına aktarılmamalıdır.

## Uygulama komutları

1. **Mekanik analizleri indir** → kullanıcıdan onay alır, 3 PDF'yi ayrı ayrı
   YFK sunucusundan cihazın özel alanına indirir; metin sayfalarını ve
   bulunan `25.xxx.xxxx` kodlarını cihazın SQLite veritabanına indeksler.
2. **Mekanik analiz durumu** → kurulu ciltleri kontrol eder.
3. **Mekanik analizde PVC boru ara** veya **Mekanik analizde
   25.100.1005 pozunu incele** → kaynak cilt, sayfa ve alıntı verir.
   Örnek numara bir test arama girdisidir; ilgili pozun mevcut olduğunu
   veya projeye uyduğunu garanti etmez.
4. DWG/DXF açıldıktan sonra **Poz eşleştir** veya **Keşif pozlarını öner** →
   `MusaAiCsbEstimate` metraj motorunun belgelediği kalem açıklamalarıyla
   analiz sayfalarında ön arama yapar. Önerilen sayfa **kesin poz değil**
   bir inceleme adayıdır.

## Veri modeli ve sınırlar

- **`pages`**: kaynak cilt, PDF sayfa numarası, özgün sayfa metni,
  Türkçe normalleştirilmiş arama metni (yalnız cihazda).
- **`positions`**: tespit edilen mekanik poz kodu, PDF sayfası,
  alıntı (yalnız cihazda).
- **`meta`**: resmi kaynak URL'si, sürüm, sayfa sayısı ve tamamlanma
  bilgisi (yalnız cihazda).
- Ön keşif girdileri `MusaAiCsbEstimate.Row` ile ilişkilidir;
  kullanıcı kontrolü olmadan `Rate` oluşturulmaz, fiyat atanmaz.
- Poz etiketleri, analiz girdileri ve teknik tarifler yorumlanırken
  montaj kapsamı, malzeme standardı, DN/PN/çap, ölçü birimi,
  bağlantı parçaları, yalıtım, işçilik ve mükerrer metraj kontrol edilmelidir.
- PDF içindeki kod tespiti ve sayfa araması **kaynak erişimi** sağlar;
  otomatik çözülmüş bileşen analizi, tam materyal sınıflandırma,
  normatif teknik uygunluk veya doğrulanmış keşif değildir.
- 2026 Ocak kitapları sabit sürümdür; aylık fiyat değişiklikleri bu
  referans sürümünü otomatik güncellemez. Fiyatlar ayrı dönemli CSV
  eşleştirme/onay mekanizmasına tabidir.

## Dağıtım ve test

Android kodu:
- `MusaAiYfkTechnicalSources.java`: resmî kaynak, komutlar, kod/taksonomi.
- `MusaAiYfkTechnicalLibrary.java`: özel indirme, SQLite sayfa/poz
  indeksi, kaynaklı arama ve metrajla konservatif aday araması.
- `MainActivity.java`: asenkron komut paneli bağlantısı.

Test: `tests/MusaAiYfkTechnicalSourcesTest.java` ve GitHub Android quality gate.

**Telefon testi gerekli:** Her cildin indirilmesi, PDF'nin cihazda açılması,
kısmi ağ hataları, depolama doluluğu, PDF metin katmanı, arama doğruluğu,
Türkçe karakterler ve büyük dosya belleği gerçek cihazda doğrulanmalıdır.
