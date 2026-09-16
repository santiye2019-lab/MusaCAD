# MusaCAD Android

MusaCAD, telefonda DWG/DXF çizimlerini açmak, incelemek, ölçmek, temel düzenlemeler eklemek ve çıktı paylaşmak için geliştirilen Android CAD uygulamasıdır.

## Mevcut özellikler

- Android dosya seçicisinden DWG ve DXF açma
- Dosya yöneticisinden desteklenen DWG/DXF dosyalarını doğrudan MusaCAD ile açma
- LibreDWG tabanlı çevrimdışı DWG → DXF dönüştürme
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
- Ekranda alan seçip seçili alanı PNG veya PDF olarak paylaşma
- Büyük dosya açma yolu için 256 MB üst sınır
- Arka planda dosya okuma/dönüştürme ve iptal desteği

## Desteklenen 2B geometri

DXF tarafında temel olarak LINE, LWPOLYLINE, POLYLINE/VERTEX, CIRCLE, ARC, ELLIPSE, TEXT, MTEXT, ATTRIB/ATTDEF, SPLINE, LEADER, HATCH, SOLID, TRACE, 3DFACE ve sınırlı DIMENSION gösterimi işlenir. BLOCK/INSERT yerleşimlerinde taşıma, döndürme, ölçek, aynalama ve iç içe blokların önemli bir bölümü desteklenir.

Her DWG/DXF nesnesinin birebir desteklendiği iddia edilmez. Dinamik bloklar, harici referanslar, bazı 3B nesneler, MINSERT dizileri ve özel/proxy nesneler sınırlı veya desteklenmeyen alanlardır.

## Netlik

Vektörel olarak çözülebilen DWG/DXF geometri ekranda bitmap büyütmek yerine yeniden çizilir. Bu nedenle zoom sırasında çizgiler ve desteklenen yazılar mümkün olduğunca net kalır. DWG dönüştürülemez ve yalnız gömülü önizleme bulunursa görüntü raster önizleme olduğu için ayrıntı seviyesi kaynak DWG önizlemesiyle sınırlıdır.

## Ölçüm

Ölçüm modu önce bir kalibrasyon ister. Çizimde uzunluğu bilinen iki nokta seçilir ve gerçek değer m, cm veya mm olarak girilir. Ardından mesafe ve alan hesapları bu ölçeği kullanır. Nokta yakalama açık olduğunda desteklenen çizgi uçları ve çoklu çizgi köşeleri seçim kolaylığı sağlar.

MusaCAD ölçümleri saha ve kontrol amaçlı yardımcı ölçümlerdir; resmi metraj veya imalat kararı öncesinde kaynak proje/ölçek ayrıca doğrulanmalıdır.

## Düzenleme ve DXF kaydetme

Vektörel olarak açılan DWG önce geçici DXF çalışma kopyasına dönüştürülür. Çizgi, çoklu çizgi, dikdörtgen, daire ve yazı düzenlemeleri ekranda tutulur ve `DXF Kaydet` ile kaynak/çalışma DXF'inin ENTITIES bölümüne eklenerek yeni bir dosyaya yazılır. Orijinal dosya değiştirilmez.

## Paylaşım ve çıktı

Paylaş menüsünde:

1. Orijinal dosyayı paylaş
2. Görünümü PDF olarak paylaş
3. Görünümü PNG olarak paylaş
4. Alan seçerek paylaş

seçenekleri bulunur. Tam görünüm PDF'i doğrudan CAD görünümünden PDF canvas'ına çizilir. Seçili alan paylaşımı ekran üzerinde dikdörtgen seçimle çalışır ve PNG/PDF önizlemesi verir.

## Büyük dosyalar

Eski 32 MB açma sınırı kaldırılmış, mevcut büyük çizim yolu 256 MB üst sınıra yükseltilmiştir. DXF ayrıştırıcısında ayrıca bellek tüketimini sınırlamak için etiket/satır sınırı bulunur. Çok büyük veya çok karmaşık dosyalarda telefonun RAM miktarı yine pratik sınır oluşturabilir.

## Lisans

DWG dönüştürme motoru LibreDWG kullanır. Uygulama içinde LibreDWG lisans metni gösterilir ve karşılık gelen kaynak paketinin CI çıktısına dahil edilmesi sağlanır. Ayrıntılar `NATIVE_BUILD.md`, `LICENSE` ve uygulamadaki `MusaCAD hakkında` ekranındadır.

## Derleme

- compileSdk: 35
- targetSdk: 35
- minSdk: 24
- Java: 17
- Android Gradle Plugin: 8.6.1
- Native DWG köprüsü: sabit commit'e pinlenmiş LibreDWG

GitHub Actions kalite kapısı native dönüştürücü smoke testini, Java yardımcı sınıf testlerini, Android lint ve unit testlerini, debug ve release varyantlarının derlenmesini çalıştırır.

## Kurulum güvenlik notu

Android, Play Store dışından elle yüklenen uygulamalarda kaynağa/cihaz politikasına bağlı olarak “bilinmeyen uygulama”, Play Protect veya benzeri bir yükleme uyarısı gösterebilir. Bu uyarı uygulama içi kodla güvenli biçimde kaldırılamaz. Kalıcı olarak uyarısız dağıtım için uygulamanın sabit bir release anahtarıyla imzalanması ve tercihen Google Play gibi güvenilen bir dağıtım kanalından yayınlanması gerekir.
