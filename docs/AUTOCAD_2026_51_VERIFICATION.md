# AutoCAD 2026 – 51 bağımsız kabul dosyası

Bu belge uygulamanın **51/51 eşdeğer** olduğunu iddia etmez.
51 aile, başlangıç kapsam denetimidir; AutoCAD 2026'nın tüm komutları,
seçenekleri, sistem değişkenleri, API'leri ve sektör araçları değildir.

## Yapılan değişiklik

- `docs/AUTOCAD_2026_ACCEPTANCE_51.json`: Mevcut 51 ailenin her birinin
  referans kabul koşulu, ilk sınama senaryosu, kapsamın tamamlanıp
  tamamlanmadığı ve sekiz ayrı kanıt kapısı.
- `tests/autocad_2026_51_acceptance.py`: Referans sonucunun, MusaCAD
  çıktısının, anlam/nesne karşılaştırmasının, bağımsız DWG yeniden açma
  denetiminin, Android/Windows/iOS cihaz kontrollerinin ve CI'nın
  gerçek dosya + SHA256 ile izlenmesini zorunlu kılar.
- Sahte onayı engellemek için eksik, yer değiştirmiş, bozulmuş veya sadece
  metinle beyan edilen sonuçları reddeder; onaylanan ailede envanter,
  senaryo, kanıt kapıları ve bağımsız inceleyen onayı tam olmalıdır.
- Bu doğrulama, tek başına üçüncü taraf referans dosyasının gerçekten
  AutoCAD 2026'dan geldiğini kanıtlayamaz. İnsan tarafından kaynak,
  gerçek yazılım sürümü ve referans üretim süreci ayrıca incelenmelidir.

## Sekiz kapı

1. AutoCAD 2026 lisanslı masaüstünde gerçek referans çalıştırma.
2. Aynı test girdisini MusaCAD ile çalıştırma.
3. Nesne, davranış ve geometriyi bağımsız olarak karşılaştırma.
4. Gerçek DWG kaydetme ve bağımsız okuyucuda yeniden açma.
5. Android fiziksel cihaz testi.
6. Windows masaüstü testi.
7. iOS fiziksel cihaz testi.
8. Başarılı sürüm-kilitli regresyon CI kanıtı.

Bir ailede son onay için test senaryolarının tüm seçenekleri kapsayacak
biçimde genişletilmiş olması gerekir. Şu anda her aile için yalnızca
başlangıç senaryosu bulunur; `scenario_inventory_complete=false`.
Bu nedenle tekil bir dar testin geçmesi AutoCAD 2026 eşdeğerliği değildir.

## Zorunlu dış test altyapısı

AutoCAD 2026 çalıştırılabilen bağımsız Windows ortamı, gerçek
DWG dosyaları ve karşılaştırmalı sonuçlar ile Android/Windows/iOS
uygulama test ortamları zorunludur. GitHub'ın sentetik Linux testleri
bu kanıtların yerine geçmez. Yazılım ve referans çıktıları olmadan
onay vermek doğru olmayacaktır. Gerçek özel proje DWG'lerini bu
herkese açık GitHub deposuna **yüklemeyin**; gizli yerel testlerde
kullanıp kişisel/veri gizliliği içermeyen kanıt özetlerini aktarın.

## Durum komutu

`python3 tests/autocad_2026_51_acceptance.py`

Bu komut, tamamlanmamış kapıları aile aile raporlar ve yalancı
onay durumunda hata vererek birleşmeyi engeller.
