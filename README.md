# MusaCAD Android

MusaCAD, telefonda DWG/DXF çizimlerini açmak, incelemek, ölçmek, temel düzenlemeler eklemek ve çıktı paylaşmak için geliştirilen Android CAD uygulamasıdır.

## Mevcut özellikler

- Android dosya seçicisinden DWG ve DXF açma
- Dosya yöneticisinden desteklenen DWG/DXF dosyalarını doğrudan MusaCAD ile açma
- LibreDWG tabanlı kalıcı native DWG oturumu ve çevrimdışı DWG → DXF düzenleme modeli
- DWG açılır açılmaz native hızlı vektör sahne; tam düzenlenebilir model hazır olunca kesintisiz geçiş
- Büyük çizimlerde zoom/pan sırasında native vektör gezinme katmanı, parmak bırakılınca tam CAD vektörü
- Dönüşüm başarısızsa DWG içindeki gömülü önizlemeyi kullanma
- Vektörel DXF görüntüleme ve yakınlaştırmada yeniden çizim
- İki parmakla zoom, tek parmakla gezinme, çift dokunmayla ekrana sığdırma
- Katman açma/kapatma
- Uç nokta/köşe yakalama
- Kalibrasyonla metre, santimetre veya milimetre cinsinden mesafe ve alan ölçümü
- Çizgi, çoklu çizgi, dikdörtgen, daire ve yazı ekleme
- Düzenlemeleri geri alma
- Düzenlenmiş çizimi DXF olarak kaydetme
- Orijinal DWG/DXF dosyasını paylaşma
- Görünümü PNG olarak paylaşma
- Tam görünümü doğrudan PDF çizim yüzeyine aktararak paylaşma
- Gelişmiş yazdırma / PDF: A4–A0, yatay/dikey, Extents/Display/Window, Sayfaya Sığdır, 1:1–1:200 ve özel ölçek, renkli/siyah-beyaz, WYSIWYG Print Preview
- Ekranda alan seçip seçili alanı PNG veya PDF olarak paylaşma
- Büyük dosya açma yolu için 512 MB üst sınır
- Arka planda dosya okuma/dönüştürme ve iptal desteği
- Kaydedilmemiş DXF düzenlemeleri için otomatik kurtarma kopyası; beklenmedik kapanma sonrası Kurtar / Sil / Sonra akışı

## ACIS 3B katılar

DXF içindeki kullanılabilir ACIS SAT verisi doğrudan B-Rep topolojisi olarak okunur. `3DSOLID`, `BODY` ve `REGION` için gerçek `face/loop/coedge/edge/vertex/point` zinciri korunur; düzlemsel yüzeyler ve yaygın konik/silindirik yan yüzeyler üçgen ağa dönüştürülür. Düz ve eliptik ACIS kenarları gerçek geometri üzerinden örneklenir. Henüz tessellate edilemeyen spline/özel eğrisel yüzeylerde uydurma dolgu yapılmaz; gerçek B-Rep sınırları tel-kafes olarak gösterilir.

ACIS gövdesi kaynak DWG→DXF dönüşümünde erişilebilir değilse MusaCAD geometri uydurmaz. Özellikle bazı yeni DWG sürümlerinde katı verisi ayrı AcDs/SAB deposunda bulunabildiğinden, native dönüştürücünün sağlayamadığı gövdelerde yalnız son çare olarak çizim extents tel-kafes fallback'i kullanılır.

## Desteklenen 2B geometri

DXF tarafında temel olarak LINE, LWPOLYLINE, POLYLINE/VERTEX, CIRCLE, ARC, ELLIPSE, TEXT, MTEXT, ATTRIB/ATTDEF, SPLINE, LEADER, HATCH, SOLID, TRACE, 3DFACE ve sınırlı DIMENSION gösterimi işlenir. BLOCK/INSERT yerleşimlerinde taşıma, döndürme, ölçek, aynalama ve iç içe blokların önemli bir bölümü desteklenir.

Her DWG/DXF nesnesinin birebir desteklendiği iddia edilmez. Dinamik bloklar, harici referanslar, bazı 3B nesneler, MINSERT dizileri ve özel/proxy nesneler sınırlı veya desteklenmeyen alanlardır.

## Netlik

Vektörel olarak çözülebilen DWG/DXF geometri ekranda bitmap büyütmek yerine yeniden çizilir. DWG dosyalarında ilk görüntü ve aktif zoom/pan sırasında LibreDWG oturumundan üretilen kompakt native vektör sahne kullanılır; hareket durduğunda tam DXF vektör modeli devralır. Böylece büyük projelerde gezinme yükü azalırken yüksek yakınlaştırmada bitmap bulanıklığına dönülmez. Native sahne üretilemeyen dosyalarda varsa gömülü DWG önizlemesi ilk görüntü olarak kullanılabilir; tam vektör model hazır olduğunda onun yerini alır.

## Ölçüm

Ölçüm modu önce bir kalibrasyon ister. Çizimde uzunluğu bilinen iki nokta seçilir ve gerçek değer m, cm veya mm olarak girilir. Ardından mesafe ve alan hesapları bu ölçeği kullanır. Nokta yakalama açık olduğunda desteklenen çizgi uçları ve çoklu çizgi köşeleri seçim kolaylığı sağlar.

MusaCAD ölçümleri saha ve kontrol amaçlı yardımcı ölçümlerdir; resmi metraj veya imalat kararı öncesinde kaynak proje/ölçek ayrıca doğrulanmalıdır.

## Düzenleme ve DXF kaydetme

Vektörel olarak açılan DWG önce geçici DXF çalışma kopyasına dönüştürülür. Çizgi, çoklu çizgi, dikdörtgen, daire ve yazı düzenlemeleri ekranda tutulur ve `DXF Kaydet` ile kaynak/çalışma DXF'inin ENTITIES bölümüne eklenerek yeni bir dosyaya yazılır. Orijinal dosya değiştirilmez.

## Paylaşım ve çıktı

Yazdır menüsü fiziksel yazıcı ve Android'in “PDF olarak kaydet” hedeflerini aynı vektörel çıktı motoruyla kullanır. Extents tüm çizimi, Display ekranda görünen alanı, Window ise kullanıcı tarafından sürüklenerek seçilen alanı basar. Print Preview ile gerçek baskı/PDF aynı render yolunu kullandığı için desteklenen çizgi, katman, renk, font, tablo, blok, tarama, resim, ölçü ve açıklamalar aynı içerikle üretilir.

Paylaş menüsünde:

1. Orijinal dosyayı paylaş
2. Görünümü PDF olarak paylaş
3. Görünümü PNG olarak paylaş
4. Alan seçerek paylaş

seçenekleri bulunur. Tam görünüm PDF'i doğrudan CAD görünümünden PDF canvas'ına çizilir. Seçili alan paylaşımı ekran üzerinde dikdörtgen seçimle çalışır ve PNG/PDF önizlemesi verir.

## Büyük dosyalar

Eski 32 MB açma sınırı kaldırılmış, dosya kopyalama ve dönüştürülmüş DXF yolu 512 MB üst sınıra yükseltilmiştir. Native hızlı sahne en fazla sınırlı bir kompakt geometri akışı tutar; tam DXF ayrıştırıcısında da bellek tüketimini sınırlayan streaming ve indeksleme yolu kullanılır. Çok büyük veya çok karmaşık dosyalarda telefonun RAM miktarı yine pratik sınır oluşturabilir.

## Lisans

DWG dönüştürme motoru LibreDWG kullanır. Uygulama içinde LibreDWG lisans metni gösterilir ve karşılık gelen kaynak paketinin CI çıktısına dahil edilmesi sağlanır. Ayrıntılar `NATIVE_BUILD.md`, `LICENSE` ve uygulamadaki `MusaCAD hakkında` ekranındadır.

## Dağıtım modelleri

MusaCAD iki ayrı Android dağıtım varyantına ayrılmıştır:

- `play`: Google Play dağıtımı. 1 günlük deneme ve mevcut/önceden etkinleştirilmiş ücretli lisans için yıllık Google Play yenilemesi bulunur. İlk ücretli aktivasyon lisans koduyla yapılır.
- `direct`: Doğrudan APK / kurumsal dağıtım. Telefona özel 12 karakterlik Serial + 12 haneli lisans kodu veya kurumsal RSA lisansı kullanılır. Bu varyant Google Play Billing kitaplığını içermez ve satın alma ekranı açmaz.

Release derleme hedefleri: `assemblePlayRelease` ve `assembleDirectRelease`. Kullanıcıya elle kurulacak final APK, aksi özellikle istenmedikçe `direct` varyantından üretilir.

## Derleme

- compileSdk: 35
- targetSdk: 35
- minSdk: 24
- Java: 17
- Android Gradle Plugin: 8.6.1
- Native DWG köprüsü: sabit commit'e pinlenmiş LibreDWG

GitHub Actions kalite kapısı native dönüştürücü smoke testini, Java yardımcı sınıf testlerini, Android lint ve unit testlerini, debug ve release varyantlarının derlenmesini çalıştırır.

## Production release güvenliği

Ticari release için `MUSACAD_PRODUCTION_RELEASE=true` kullanılır. Bu mod release imzası, HTTPS trial servisi, trial public key'i ve offline paid-license public key'i eksikse build'i durdurur. Play varyantında server-side Play doğrulama endpoint'i de zorunludur.

Production build'de 12 haneli legacy kısa kod aktivasyonu kapalıdır. Ticari offline lisanslama, cihazın **Güvenli Lisans Kimliği** için RSA imzalı `MC1` token kullanır. Ayrı `licensemanager` modülü MC1 özel anahtarını Android Keystore ile cihazda sarar ve parola korumalı AES-GCM `.mlk` yedeği ile yeni telefona kurtarabilir. Aktif License Manager anahtar fingerprint'i ana MusaCAD public key'iyle eşleşmiyorsa MC1 üretimi engellenir.

Ayrıntılar: `RELEASE_SIGNING.md`, `LICENSE_MANAGER.md` ve gerçek cihaz final kontrolü için `DEVICE_FINAL_QA.md`.

## Kurulum güvenlik notu

Android, Play Store dışından elle yüklenen uygulamalarda kaynağa/cihaz politikasına bağlı olarak “bilinmeyen uygulama”, Play Protect veya benzeri bir yükleme uyarısı gösterebilir. Bu uyarı uygulama içi kodla güvenli biçimde kaldırılamaz. Kalıcı olarak uyarısız dağıtım için uygulamanın sabit bir release anahtarıyla imzalanması ve tercihen Google Play gibi güvenilen bir dağıtım kanalından yayınlanması gerekir.


## DWG görüntü doğruluğu
- İlk Native DWG karesinde LibreDWG ByLayer / ByBlock / ACI / TrueColor renk çözümleme; tam DXF modele geçişte renk sıçramasını önleme
