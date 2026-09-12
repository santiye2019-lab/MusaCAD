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

DWG kapalı ve karmaşık bir formattır. Bu prototip DWG dosyasındaki gömülü önizlemeyi gösterir; ASCII DXF dosyalarında ise temel 2B geometrileri doğrudan okur. Katmanlar, bloklar, yazılar ve hassas nesne yakalama için daha kapsamlı bir CAD motoru bağlanmalıdır. Ekran üzerinden yapılan ölçümler kontrol amaçlıdır; resmi metraj öncesinde doğrulanmalıdır.

## Derleme

Android Studio'da klasörü açın ve `app` modülünü çalıştırın. Proje Java 8, minSdk 24 ve targetSdk 35 kullanır.
