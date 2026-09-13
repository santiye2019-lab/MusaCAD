# Ücretsiz DWG motoru denemesi — 13 Eylül 2026

## Amaç ve karar

MusaCAD'in mevcut LibreDWG okuyucusuna alternatif olarak LibreCAD libdxfrw
bilgisayarda derlenip denendi. Kullanıcının yerel DWG dosyasını bu okuyucu da
açamadığı için Android uygulamasına ikinci motor eklenmedi. Sonuç, dosyanın
bozuk olduğunu kanıtlamaz; başka bir CAD uygulamasıyla karşılaştırma gerekir.
Kullanıcı dosyası ve dosyadan çıkan günlükler bu depoya yüklenmedi.

## Sabit kaynaklar

- libdxfrw: https://github.com/LibreCAD/libdxfrw/tree/92d7466ed9146badcd4fb44c82d1dd8302b3c7db
- Kontrol örneği: LibreDWG `test/test-data/example_2013.dwg`,
  sürüm `34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43`.
- libdxfrw kaynak başlıkları GPL-2.0-or-later lisansını belirtir.

## Tekrarlama

libdxfrw kaynak dizininde Linux ve g++ ile:

```sh
g++ -std=c++14 -O1 -Isrc src/*.cpp src/intern/*.cpp \
  dwg2dxf/dx_iface.cpp dwg2dxf/main.cpp -o ../libdxfrw-converter
```

Her denemede henüz mevcut olmayan ayrı bir çıktı yolu kullanın:

```sh
timeout 60 ./libdxfrw-converter input.dwg -v2007 output.dxf
```

## Gözlenen sonuçlar

| Girdi | Sonuç |
| --- | --- |
| Kamusal DWG 2013 kontrol örneği | Çıkış kodu 0, yaklaşık 265 KiB ASCII DXF oluştu. |
| Yerel kullanıcı DWG 2013 dosyası | Çıkış kodu 1, `BAD_READ_FILE_HEADER` (5), DXF oluşmadı. |

Kontrol çıktısında altı SECTION/ENDSEC çifti, bir EOF, 68 LINE,
11 LWPOLYLINE, bir CIRCLE, beş ARC ve on INSERT kaydı bulundu. Bunlar yalnız
çıktı yapısı kontrolleridir; çizimin özgün dosyayla görsel veya ölçüsel olarak
birebir eşleştiğini göstermez. Dönüştürücü ayrıca bir `extmax` tanılama satırı
yazdırdı; uygulamaya entegrasyon öncesinde sınır kutusu doğruluğu incelenmelidir.

Bu deneme Android cihaz testi değildir. Mevcut LibreDWG motorunun aynı yerel
dosyada bölüm haritasını okuyamamasıyla birlikte değerlendirildiğinde, ikinci
motor bu dosyadaki engeli çözmemiştir. Sonraki doğrulama için dosyanın başka bir
CAD uygulamasında açılması ve yeniden kaydedilmiş DWG veya ASCII DXF kopyasıyla
karşılaştırılması gerekir.
