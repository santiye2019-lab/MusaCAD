# MusaCAD — Gerçek Cihaz Final QA

Bu liste otomatik CI testlerinin yerine geçmez; release öncesi gerçek Android cihazında uygulanır.

## 1. Temiz kurulum ve lisans
- [ ] Uygulamayı daha önce MusaCAD kurulmamış bir cihaz/profilde kur.
- [ ] Açılış ve Hakkında ekranlarının farklı ekran oranında taşmadığını doğrula.
- [ ] 1 günlük deneme servisinin production endpoint üzerinden başladığını doğrula.
- [ ] Deneme bittikten sonra yeniden kurulumun yeni deneme vermediğini doğrula.
- [ ] Lisans Bilgisi ekranından **Güvenli Lisans Kimliği** kopyalanabiliyor.
- [ ] License Manager ile bu kimlik için RSA imzalı MC1 lisansı üret.
- [ ] MC1 lisansını MusaCAD'e yapıştır ve aktivasyonu doğrula.
- [ ] Yanlış cihaz için üretilmiş MC1 token reddediliyor.
- [ ] Süresi geçmiş MC1 token reddediliyor.
- [ ] Production build 12 haneli legacy kısa kodu reddediyor.

## 2. DWG / DXF saha dosyaları
En az üç gerçek proje kullan:
1. küçük/orta 2B plan,
2. büyük ve çok katmanlı DWG,
3. 3B / ACIS 3DSOLID içeren örnek.

- [ ] Dosya yöneticisinden MusaCAD ile açma.
- [ ] Uygulama içinden dosya seçerek açma.
- [ ] Yakınlaştırmada çizgiler keskin kalıyor.
- [ ] Pan/zoom sırasında çökme veya ekran kararması yok.
- [ ] Layer aç/kapat doğru.
- [ ] Bloklar, yazılar, SHX/TTF fontlar ve renkler kaynak çizime yakın.
- [ ] OLE/Excel önizlemeleri mevcut dosyalarda görünür.
- [ ] 3DSOLID desteklenen SAT/B-Rep verisinde gerçek yüzey olarak görünür.
- [ ] ACIS ölçümü çalışır; ACIS tessellation köşe düzenlemesi salt-okunur kalır.

## 3. Ölçüm ve düzenleme
- [ ] Bilinen 1 m / 5 m / 10 m referanslarla kalibrasyon kontrolü.
- [ ] Mesafe ve alan ölçümü.
- [ ] Endpoint/köşe snap.
- [ ] Çizgi, polyline, dikdörtgen, daire, yazı ekle.
- [ ] Undo.
- [ ] DXF kopyasını kaydet ve yeniden aç.
- [ ] Beklenmedik kapanma sonrası recovery akışı.

## 4. Yazdırma ve paylaşım
- [ ] Extents / Display / Window.
- [ ] A4, A3 ve en az bir büyük format.
- [ ] Portrait / Landscape.
- [ ] Fit to page ve 1:50 / 1:100.
- [ ] Renkli / siyah-beyaz.
- [ ] Print Preview ile gerçek PDF aynı içerikte.
- [ ] Android **PDF olarak kaydet**.
- [ ] Fiziksel yazıcı varsa bir test baskısı.
- [ ] PNG/PDF paylaşımı ve seçili alan paylaşımı.

## 5. Performans / dayanıklılık
- [ ] Büyük DWG açılışında cihaz bellek uyarısı/çökme yok.
- [ ] Uygulamayı arka plana alıp geri dön.
- [ ] Ekranı kapat/aç; dosya durumu korunuyor.
- [ ] 30 dakika karma kullanımda ANR/çökme yok.
- [ ] Düşük pil / uçak modu senaryosunda offline MC1 lisansı çalışıyor.

## Release onayı
Tüm maddeler gerçek cihaz üzerinde işaretlenmeden **production APK/AAB final** sayılmaz.
