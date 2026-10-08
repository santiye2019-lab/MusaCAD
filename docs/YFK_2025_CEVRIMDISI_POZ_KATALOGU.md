# MusaCAD — 2025 YFK Çevrim Dışı Poz, Tarif ve Birim Fiyat Arşivi

## Amaç ve kullanıcı akışı

2025 Yüksek Fen Kurulu *İnşaat ve Tesisat Birim Fiyatları* kitabını
MusaCAD'de **kullanıcının kendi PDF kopyasından**, internetsiz çalışan
yerel poz veritabanına dönüştürmek. Kaynağın tamamı cihazın Android
uygulama-özel depolama alanında saklanır. **Kitap PDF'si, tariflerinin
toplu dökümü ve türetilmiş veritabanı bu GitHub deposunda bulunmaz.**

Uygulamada şu komutları kullanın:

1. **`2025 kitabını yükle`** — Android PDF dosya seçicisini açar. Kullanıcı
   kendi 2025 YFK kitabını seçer. Yalnızca bu cihazda PDF ve SQLite
   indeks oluşturulur; internet gerekmez. İşlem uzun sürebilir.
2. **`2025 katalog durumu`** — yerel indeks sayfa/poz sayılarını gösterir.
3. **`2025 poz 25.175.1401`** veya **`2025 kitabında lavabo ara`** —
   kaynak PDF sayfasını, pozun yerel açıklama özetini, açıkça bulunan
   ölçü birimini ve **2025-01** fiyat/ayrı montaj bedeli sütunlarını getirir.
4. **`2025 2026 yeni pozları tara`** — hem bu 2025 arşivi hem de eski
   `ÇŞB kitabını indir` komutu ile kurulmuş 2026 Ocak PDF arşivi
   varsa, kaynaklardaki poz kodlarını karşılaştırır; 2025'te görünmeyen
   2026 kodlarını **yeni poz adayları** olarak listeler.
   Fiyatlandırma yapmaz, pozların gerçekten yeni olduğunu iddia etmez.

Örnek kodlar arama örneğidir; mühendislik eşleşmesi otomatik onaylanmaz.

## Veriler

`MusaAiYfk2025Library` özel SQLite içinde şunları saklar:
- `items.code`: PDF metin katmanından algılanan poz kodu.
- `items.description`: **sayfa içi** teknik tarif/kalem özeti; bazı
  iş kalemlerinin devam eden tarifleri farklı sayfalarda olabilir.
- `items.unit`: yalnız açıkça tanınan ölçü birimi. Boşsa belirsiz.
- `items.price_2025`: 2025 kaynak baskısındaki ilk parasal sütun.
- `items.mounting_2025`: PDF'de varsa montaj bedeli sütunu.
- `items.page` ve `pages.text`: referans olarak özgün PDF sayfası
  ve tam sayfa metni (yalnız cihazda).

Bu bilgiler kural tabanlı OCR/PDF metin ayrıştırma adaylarıdır; her
satırın tarifi ve fiyatı güncel mevzuat/şartname ile ayrıca
doğrulanmalıdır. İlk fiyat alanı farklı kısımlarda
**rayiç**, **birim fiyat** veya **montajlı birim fiyat** olabilir;
dolayısıyla bu alanın kapsamı yalnız sayfa bağlamında anlaşılır.
MusaCAD, bu sayıları `MusaAiCsbEstimate.Rate` üzerinde
`userVerifiedInstalledPrice=true` olarak otomatik işaretlemez.
`2025-01` fiyatını 2026 fiyatına yükseltmez.

## Analiz ve 2026 ile ilişki

2025 kitabı poz/rayiç listesini ve bazı imalat tariflerini içerir;
**ayrı yayımlanan tam poz analiz ciltlerinin yerini tutmaz.**
2026 mekanik analiz araması başka `MusaAiYfkTechnicalLibrary`
modülündedir; kullanıcı isterse ilgili ciltleri resmî kaynaktan
indirerek ayrıca cihaza kurar.

`compareNew2026`, 2026 PDF'de satır başlangıcında tespit edilen
kodları 2025 özel dizinindeki kodlarla karşılaştırır. PDF metin katmanı
hatları ve poz revizyonları nedeniyle farklar **aday** seviyesindedir.
Onaysız yeni imalat fiyatı veya teknik tarifi oluşturulmaz.

## Yeniden dağıtım ve güvenlik

Yüklenen 2025 kitabının 2. sayfasında, Bakanlığın yazılı izni olmadan
eserin tamamının veya bir kısmının işlenmesi, çoğaltılması ve
elektronik dağıtımı için hakların saklı olduğu belirtilir. MusaCAD'in
**public** deposuna kitabın, poz-tarif veritabanının veya bütünsel fiyat
cetvelinin eklenmesi bu modülde yapılmaz. Yayının ticari olarak
uygulamayla dağıtılması planlanırsa Bakanlıktan yazılı izin alınmalıdır.

- PDF yalnız kullanıcının Android dosya seçicisinden gelir.
- PDF ve SQLite hiçbir MusaCAD ağına gönderilmez.
- Dosyalar uygulama özel alanında kalır; cihazda internet olmadan aranabilir.
- Uygulama silinince özel depolama da silinebilir; yedek için
  kullanıcının orijinal PDF'sini ayrıca koruması gerekir.
- İndeks büyük bir işlem olduğundan arka plan görevinde yapılır.
- PDF metin katmanı eksik dosyalar güvenilir katalog oluşturmaz.

## Uygulama dosyaları

- `MusaAiYfk2025Parser.java`: fiyat sütunlarını ayıran saf Java parser
- `MusaAiYfk2025Library.java`: özel PDF + SQLite kitaplık/karşılaştırma
- `MainActivity.java`: dosya seçicisi + Gandalf doğal dil komutları
- `tests/MusaAiYfk2025ParserTest.java`: sentetik veriyle regresyon testi

**Kalan saha kontrolleri:** Kullanıcının 675 sayfalık gerçek PDF'sini
telefon üzerinde bir kez içe aktararak sayfa/poz sayılarını doğrulama;
birimlerin grup tariflerinden geldiği durumlar, çok satırlı tarifler,
PDF açılış performansı ve çevrim dışı kullanım kontrolü.
