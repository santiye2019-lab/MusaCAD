# MusaCAD — gerçek AC1024/R2010 çizimlerinde OT505 = MATERIAL (sınırlı kanıt)

**Tarih:** 2026-11-06
**PR:** https://github.com/santiye2019-lab/MusaCAD/pull/720
**Doğrulanan ilk-taraf test başı:** b628a3a134e5181bba4718628f9a2d177d3dc83e
**Kaynak doğrulama:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42124302570
**Tanı artifact'ı:** musacad-real-dwg-native-stage-diagnostic / musacad-real-classes-numeric-census.json

## Kesin olarak gösterilenler

- Git LFS kaynak SHA denetimli gerçek DWG matrisinde toplam **23 dosya** kontrol edildi; **10 tanesi AC1024**. Dokuz Karagümrük dosyasının orijinal AcDb:Classes fiziksel sayfaları doğrulandı ve mantıksal Classes aralığı çıkarıldı.
- Bu dokuz dosyada bağımsız MusaCAD C11 kodu ile **532 sayısal sınıf kaydı**, bunlara karşılık gelen **1.596 Unicode TU sınıf alanı** ve **31.240 UTF-16 kod birimi** sınır/bütünlük denetiminden geçti.
- **9/9** dosyada çizim-yerel sınıf numarası **505** bulundu; item_class_id **0x1F3 (OBJECT)**.
- Aynı dokuz dosyanın tamamında hedef kayıt için üç TU alanı boş değildi, ASCII olarak doğrulandı ve üçüncü TU alanından yalnızca güvenli DXF sınıf tanımlayıcısı **MATERIAL** çıktı. Her kaynak için 8 karakterlik tam eşleşme elde edildi.
- Orijinal DWG dosyaları değiştirilmedi. Tam çizimler, koordinatlar, layer adları, handle'lar, TEXT içeriği ve sınıfların diğer metinleri tanı artifact'ına yüklenmedi.
- İlk-taraf çekirdekte musa_dwg_ac1024_classes_r2010_tu_audit ve musa_dwg_ac1024_classes_r2010_dxf_identifier vardır. Birincisi tüm TU akışını fail-atomic doğrular; ikincisi **açık hedef sınıf** için en fazla 96 ASCII tanımlayıcı karakterine izin verir. Başka sınıfın adı otomatik ilan edilmez.

## Bu kanıttan ÇIKARILMAYACAK sonuçlar

1. **MATERIAL**, DXF sınıf tanımlayıcısıdır; material asset verileri, yüzey renkleri, BYLAYER etkisi veya malzeme referans grafiğinin doğruluğu **kanıtlanmamıştır**.
2. **505 sayısı sadece gözlenen dokuz DWG içinde bu hedefe aittir.** Başka bir DWG dosyasındaki 505 aynı sınıf kabul edilemez; her dosyanın orijinal Classes bölümü ayrıca doğrulanmalıdır.
3. LAYER OT51 son handle akışında görülen OT80 / OT505 / OT57 komşuluğu, tek başına çizimdeki nesnelerin görünürlüğünü veya malzeme/plotstyle/linetype bağlantılarının semantiğini çözmez. Henüz görünürlük, renk, blok yerleşimi, kesin geometri eşdeğerliği veya DWG yazımı kabulü yoktur.
4. source_classes_r2010_505_dxf_identifier_verified alanı **yalnızca doğrulanmış string kimliğine** işaret eder. class_semantics_accepted=false, rendering_verified=false, geometry_verified=false.
5. İzole LibreDWG host oracle yalnızca geçici CI referans bilgisayarı içindir; MusaCAD çalışma zamanı çekirdeğine veya APK'ya dahil edilmemiştir.

## Sonraki teknik eşik

- OT51 LAYER kayıtlarının ilgili H referanslarını ham tür ve CRC/self-H doğrulamasıyla devam ettir.
- OT80 ve OT57'nin tam tür kimliğini kaynağın orijinal Classes/nesne tip tanımıyla göster; OT505 için kanıtlanan **MATERIAL** bilgisini yalnızca açıkça doğrulanmış kaynak grafına bağla.
- Bundan sonra katman açık/kapalı, dondurulmuş, kilitli, BYLAYER/BYBLOCK renk ve çizgi tipi davranışlarını gerçek DWG görünürlük/render testiyle doğrula.
- Ana dala birleştirmeyi, normal Android DWG açmayı ve final APK iddiasını ilgili bağımsız testler geçene kadar kapalı tut.

**Biçim referansı:** Open Design Specification for .dwg files, R2007/R2010 Classes ve TU akış tarifleri; kod ilk-taraf C11'dir, referans kütüphane kodu kullanılmamıştır.
