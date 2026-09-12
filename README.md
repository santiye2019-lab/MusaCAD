# Musa CAD (Android)

İlk prototip özellikleri:

- Android dosya seçiciden DWG/DXF seçme
- DWG içine gömülü önizlemeyi açma
- ASCII DXF içindeki LINE, LWPOLYLINE, CIRCLE ve ARC geometrilerini çizme
- Yakınlaştırma ve kaydırma
- Kalibrasyonla gerçek birimde mesafe ve alan ölçme
- Geri alma ve ölçümü temizleme
- Orijinal dosyayı paylaşma
- Görünümü PNG olarak paylaşma
- Görünümü PDF olarak oluşturup paylaşma

## Önemli teknik not

DWG kapalı ve karmaşık bir formattır. Bu prototip DWG dosyasındaki gömülü önizlemeyi gösterir; ASCII DXF dosyalarında ise temel 2B geometrileri doğrudan okur. Tam DWG geometrisi için ayrıca bir CAD motoru gerekir. DXF yazı, blok ve nokta yakalama desteği aşağıdaki kapsamla sınırlıdır. Ekran üzerinden yapılan ölçümler kontrol amaçlıdır; resmi metraj öncesinde doğrulanmalıdır.

## Derleme

Android Studio'da klasörü açın ve `app` modülünü çalıştırın. Proje Java 8, minSdk 24 ve targetSdk 35 kullanır.

## Bu geliştirme

- TEXT ve MTEXT için düz yazı gösterimi; parçalı MTEXT içeriği, satır sonları ve Unicode kaçışları okunur.
- Katman adına göre ayırt edici renkler atanır. Bunlar dosyanın özgün renkleri değildir.
- Açılan nesne ve katman sayısı, atlanan nesne sayısıyla birlikte gösterilir.
- Ölçüm noktaları görüntü koordinatlarında tutulur; zoom ölçeği ölçüm sonucuna katılmaz.
- Yeni çizimde kalibrasyon sıfırlanır; kalibrasyonsuz ölçüler piksel olarak belirtilir.

Sınırlar: DXF hâlen 2400×2400 bitmap olarak görüntülenir; vektör yakınlaştırma,
özgün yazı tipi/hizalama/satır kaydırma ve katman görünürlüğü henüz yoktur; blok desteğinin sınırları aşağıdadır. Nokta yakalama aşağıdaki nesnelerle sınırlıdır. DWG desteği gömülü önizlemeyle sınırlıdır. APK derlenmesi, cihaz üzerinde doğruluk testi anlamına gelmez.


## Seçili alan paylaşımı

Paylaş → Alan seçerek paylaş yolunu açın. Görünümü tek parmakla sürükleyerek
dikdörtgen seçin; önizlemeden PNG veya PDF paylaşımını seçin. Yeniden Seç ile
seçimi tekrarlayabilir, seçim sırasında GERİ ile iptal edebilirsiniz.
Çizim ve mevcut ölçüm işaretleri çıktıya alınır; sarı seçim çerçevesi alınmaz.
Çıktı ekran çözünürlüğündedir; PDF, görüntü tabanlıdır ve ölçekli pafta değildir.
Her paylaşım ayrı bir geçici dosya üretir.


## DXF uç noktası yakalama

NOKTA YAKALA seçeneği, desteklenen DXF çizgilerinin uçlarına ve LWPOLYLINE
köşelerine dokunmayı kolaylaştırır. Mesafe, alan ve kalibrasyonda en yakın aday
18 dp ekran mesafesi içinde seçilir; yakalanan son nokta beyaz kareyle gösterilir.
İstenirse kapatılabilir. DWG önizlemesinde ve aday bulunmayan dosyada devre dışıdır.
Yakınlaştırmada dokunma toleransı ekran üzerinde sabit kalır.

Bu aşama kesişim, orta nokta, daire merkezi yakalamaz. Desteklenen bloklardaki çizgi uçları ve çoklu çizgi köşeleri yakalanır.
Ölçü birimini otomatik belirlemez; gerçek uzunluk için kalibrasyon gerekir.


## Temel 2B blok desteği

BLOCKS bölümündeki tanımlar, ENTITIES içindeki INSERT yerleşimleriyle açılır.
Taban noktası, taşıma, döndürme, farklı X/Y ölçekleri, aynalama ve iç içe bloklar
hesaba katılır. Blok içindeki 0 katmanı yerleşimin katmanını devralır.
Çizgi uçları ve çoklu çizgi köşeleri dönüşümden sonra nokta yakalamaya katılır.
Daire/yaylar dönüşümlü yollarla çizilir; farklı X/Y ölçeğinde eliptik görünür.

Sınırlar: MINSERT dizileri, harici referanslar, 3B yerleşimler, dinamik blok
davranışı ve öznitelikler desteklenmez. Eksik veya döngüsel blok referansı atlanan
sayısına eklenir. En fazla 32 iç içe blok ve 100.000 genişletme adımı işlenir.
Açılan nesne sayısı, blokların içinden çıkan desteklenen öğeleri de içerir.

Teknik başvuru: https://ezdxf.readthedocs.io/en/stable/blocks/insert.html


## Arka planda dosya açma

Dosya kopyalama ve çizim hazırlama tek arka plan iş parçacığında yürür.
Açılış penceresi okunan MB değerini, ardından hazırlama aşamasını gösterir.
İptal edilen veya eski kalan sonuç ekrana uygulanmaz. Dosya başarıyla açılana
kadar mevcut çizim, kalibrasyon, ölçümler ve orijinal paylaşım dosyası korunur.
Boş/okunamayan dosyada kısmi geçici kopya silinir.

Bu sürümde dosya sınırı 32 MB, DXF metin sınırı 600.000 satırdır.
DWG önizlemesi en fazla 2400 piksel kenara örneklenir.
İptal arayüzü hemen kapanır; bulut sağlayıcısının engellenen okuması dönene kadar
arka plan işinin sona ermesi gecikebilir. PDF/PNG dışa aktarımı henüz arka plana
taşınmamıştır. Telefon üzerinde bellek ve yaşam döngüsü testi yapılmalıdır.
