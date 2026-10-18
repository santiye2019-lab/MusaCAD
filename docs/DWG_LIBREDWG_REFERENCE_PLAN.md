# DWG işlevsel eşdeğerlik referansı — MusaCAD Core

**Amaç:** LibreDWG'nin kaynak kodunu kopyalamak değil; kamuya açık davranış ve format dokümantasyonuna dayanarak aynı gerçek DWG girdilerini bağımsız MusaCAD C çekirdeğinde doğrulanabilir şekilde çözmek ve yazmak. Matris: [DWG_LIBREDWG_REFERENCE_MATRIX.json](DWG_LIBREDWG_REFERENCE_MATRIX.json).

**Kritik ayrım:** GNU LibreDWG [proje README'si](https://github.com/LibreDWG/libredwg/blob/master/README) DWG okumada geniş desteği, ileri düzey bazı R2010+ nesnelerde eksikleri ve modern sürüm yazımındaki CRC sorunlarını bildirir. [GNU kılavuzu](https://www.gnu.org/software/libredwg/manual/html_node/Encoding.html) R2000 yazımını tercih eder; R2007 yazma desteğini tamamlanmış saymaz. LibreDWG de yüzde 100 bağımsız doğruluk referansı değildir.

MusaCAD içindeki `AC1032` tanıma işlevi yalnızca altı baytlık DWG sürüm imzasını ayırt eder. `MC1032` ise MusaCAD'ın ayrı, özgün `.mcad` biçimidir. Bunları karıştırmak yasaktır.

## Test sıralaması

1. **Referans:** Aynı lisanslı ve gerçek ikili DWG fixture dosyasını LibreDWG ile host test ortamında aç, nesne envanteri ve olası hata/atlama kayıtlarını çıkar. Referans kodu çalışma zamanına bağlama.
2. **Bağımsız MusaCAD:** `core/native/` altında özgün okuma yoluyla katmanları, orijinal handle kayıtlarını, geometriyi, metinleri, blok/XREF ilişkilerini, layout'ları ve 3B sınıfları çöz. Desteklenmeyen türleri açıkça bildir; sessizce atlama.
3. **Karşılaştırma:** Sayıların yanı sıra nesne bazında tür/handle/katman, dünya koordinatları, renk, UCS/OCS, proxy ve bitişik ilişkilerini karşılaştır. LibreDWG ile uyuşma tek başına AutoCAD 2026 kabulü değildir.
4. **Bağımsız yazma:** MusaCAD'ın yazdığı **gerçek DWG**'yi bağımsız AutoCAD ortamında yeniden aç; kayıp, bozuk handle, eksik layout/blok veya checksum hatalarını reddet. Kaynak dosyanın üzerine güvenilir kabul olmadan yazma.
5. **Üç platform:** Android NDK, Windows, iPhoneOS aynı C/C++ kaynak seti, tek C ABI, aynı test fixture sonuçları; fiziksel cihaz testleri ayrıca zorunlu. Android Java, Swift ve Windows UI ikinci bir CAD motoru içeremez.

**Kabul kanıtı**: gerçek ikili fixture SHA-256, LibreDWG referans sürümü/commit SHA, MusaCAD commit SHA, tam nesne fark dökümü, kaynak korunması, yeniden açma, platform testleri, bağımsız inceleme. Kanıt yoksa durum `not_run` kalır.

**Lisans:** LibreDWG GPLv3+ lisanslıdır. Kodun doğrudan alınması lisans ve telif yükümlülükleri doğurur. Bizim belirlenmiş ürün mimarimiz, sıfır LibreDWG CAD runtime bağımlılığı ve özgün birinci taraf çekirdektir.
