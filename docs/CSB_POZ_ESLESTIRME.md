# MusaCAD — ÇŞİDB/YFK poz eşleştirme CSV'si

Bu şablon, Yüksek Fen Kurulunun ham XLSX/PDF yayınları değildir; **yetkili kişi tarafından resmi kaynakla doğrulanmış kalemler** için kullanılacak bağımsız bir eşleştirme çizelgesidir.

1. MusaCAD'de projeyi açıp **“Projeyi analiz et”** komutuyla otomatik metrajı oluşturun. Her satırdaki `poz eşleştirme anahtarı` değerini aynen kopyalayın (ör. `BORU|Pis su | PVC | DN100`).
2. `item_key`: MusaCAD metraj eşleştirme anahtarı; `poz`: YFK'nin doğrulanmış resmi poz kodu; `aciklama`: tam resmi tarif; `birim`: **m** veya **Ad**; `birim_fiyat`: ilgili fiyat döneminde, malzeme ve montaj kapsamı teyit edilmiş TL birim fiyatı.
3. `donem` sütunu `YYYY-MM` biçimindedir, ör. **2026-10**; tek CSV'deki bütün satırlar aynı dönemden olmalıdır.
4. `kaynak`: satırın fiyat/tarifini gerçekten destekleyen **https://yfk.csb.gov.tr/** veya **https://webdosya.csb.gov.tr/** bağlantısı. Genel kurum ana sayfası yerine mümkünse ilgili resmî yayıma doğrudan bağlantı kullanın.
5. `fiyat_kapsami`: yalnız pozun tarifinde malzeme+montajın birlikte dâhil olduğu doğrulanırsa `malzeme+montaj` yazın; ham malzeme rayiçlerini bu biçimde içe aktarmayın. KDV, yüklenici kârı/genel gider ve varsa ayrıca ödenecek kalemleri poz tarifine göre doğrulayın.
6. CSV'yi UTF-8, **noktalı virgülle ayrılmış** kaydedin. Açık projede **“ÇŞİDB fiyat listesi yükle”** deyin, dosyayı seçin; onay ekranındaki dönemi/kaynağı karşılaştırdıktan sonra **“Kontrol ettim, kullan”** düğmesini seçin.
7. **“ÇŞİDB fiyatlarını temizle”** komutu proje fiyat eşleştirmelerini sıfırlar. Resmî yayınların güncelliğini dönem bazında ayrıca kontrol edin.

**Sınırlamalar:** Bir fiyatın resmî bir URL ile yazılmış olması, o kalemin poz tarifiyle DWG imalatının gerçekten örtüştüğünü tek başına kanıtlamaz. Boru basınç sınıfı, üretim standardı, fittings, bağlantı, yalıtım, montaj ve mükerrer metraj ayrıca doğrulanmalıdır. Uygulama nihai idari yaklaşık maliyet onayı vermez.

Şablon: `docs/csb-poz-eslestirme-sablon.csv`
