# MusaCAD 1.0 — Final QA Checklist

## Dosya açma
- [x] Android dosya seçicisinden DWG açma
- [x] Android dosya seçicisinden DXF açma
- [x] Desteklenen DWG/DXF dosyalarını dış uygulama/dosya yöneticisinden MusaCAD ile açma
- [x] Büyük çizim yolunda eski 32 MB sınırını aşma (256 MB üst sınır)
- [x] Açma işlemini iptal edebilme

## Görüntüleme
- [x] Vektörel DXF çizimi
- [x] DWG → DXF çevrimdışı dönüşüm
- [x] DWG dönüşümü başarısızsa gömülü önizleme
- [x] Pan
- [x] Pinch zoom
- [x] Yakınlaştır / uzaklaştır düğmeleri
- [x] Ekrana sığdır
- [x] Yakınlaştırmada vektörel yeniden çizim
- [x] Katman görünürlüğü

## Ölçüm
- [x] Ölçek kalibrasyonu
- [x] Mesafe ölçümü
- [x] Alan ölçümü
- [x] Uç nokta / köşe yakalama
- [x] Ölçüm temizleme ve geri alma

## Düzenleme
- [x] Çizgi
- [x] Çoklu çizgi
- [x] Dikdörtgen
- [x] Daire
- [x] Yazı
- [x] Düzenleme geri alma
- [x] Düzenlenmiş DXF kaydetme

## Paylaşım / çıktı
- [x] Orijinal dosyayı paylaşma
- [x] Tam görünüm PNG
- [x] Tam görünüm PDF
- [x] Tam görünüm PDF'ini CAD canvas üzerinden oluşturma
- [x] Alan seçimi
- [x] Seçili alan PNG
- [x] Seçili alan PDF

## Arayüz
- [x] MusaCAD logo / launcher icon
- [x] Açılış ekranı
- [x] Renkli araç düğmeleri ve ikonlar
- [x] Seçili mod görünümü
- [x] Dosya adı ve çizim durumu
- [x] Haptik düğme geri bildirimi

## Teknik kalite kapısı
- [x] LibreDWG native smoke test
- [x] DXF text decoding test
- [x] Selection bounds test
- [x] Endpoint snapping test
- [x] DXF block expansion test
- [x] File transfer / cancellation / large drawing regression test
- [ ] Android lintDebug
- [ ] Gradle unit tests
- [ ] assembleDebug
- [ ] assembleRelease

Son dört madde GitHub Actions kalite kapısının başarılı tamamlanmasıyla işaretlenecektir. APK kullanıcıya yalnız bu kalite kapısı geçtikten sonra verilecektir.
