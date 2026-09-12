# Musa CAD (Android)

İlk prototip özellikleri:

- Android dosya seçiciden DWG/DXF seçme
- DWG içine gömülü önizlemeyi açma
- Yakınlaştırma ve kaydırma
- Nokta seçerek mesafe ve alan işaretleme
- Geri alma ve ölçümü temizleme
- Orijinal dosyayı paylaşma
- Görünümü PNG olarak paylaşma
- Görünümü PDF olarak oluşturup paylaşma

## Önemli teknik not

DWG kapalı ve karmaşık bir formattır. Bu prototip DWG dosyasındaki gömülü önizlemeyi gösterir. Gerçek çizim koordinatları, katmanlar, nesne yakalama ve ölçekli ölçüm için `CadDocumentProvider` benzeri bir soyutlama arkasına ODA Drawings SDK, AutoCAD Platform Services veya Android için derlenmiş LibreDWG tabanlı bir motor bağlanmalıdır. Ekran önizlemesi üzerinden gösterilen ilk ölçümler bu nedenle resmi metraj kabul edilmemelidir.

## Derleme

Android Studio'da klasörü açın ve `app` modülünü çalıştırın. Proje Java 8, minSdk 24 ve targetSdk 35 kullanır.
