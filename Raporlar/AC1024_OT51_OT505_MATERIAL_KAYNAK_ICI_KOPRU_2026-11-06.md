# MusaCAD AC1024 — OT51 LAYER → OT505 MATERIAL kaynak-içi kanıt köprüsü

**Tarih:** 2026-11-06  
**PR:** https://github.com/santiye2019-lab/MusaCAD/pull/720  
**Önceden geçen tam kaynak testi:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42124302570  
**Yeni kanıt kodu:** tests/real_dwg_layer_class_bridge.py  
**Yeni CI aşaması:** .github/workflows/real-dwg-native-stages.yml  
**Yeni CI doğrulaması:** Bu yeni aşamanın sonuçları ayrıca kontrol edilmelidir; önceki yeşil CI, sonraki commitlerin doğrulaması değildir.

## Kaynaktan doğrulanmış dar ilişki

- 23 kaynak çizim içinden Karagümrük'ün beş AC1024 dosyasında incelenen toplam 14 adet OT51 LAYER örneğinde, önceki bağımsız C11 okuyucu özgün CRC/self-H ile üç terminal H hedefini doğruladı.
- İkinci, üçüncü ve dördüncü terminal H referanslarının ham OT türleri sırasıyla **80, 505, 57** (14/14). 42/42 hedef ofseti ve nesne kimliği kaynak içinde doğrulandı.
- Aynı kaynak çizimlerin kendi fiziksel AcDb:Classes bölümünden doğrulanmış sınıf numarası 505'in türü **OBJECT** ve DXF adı **MATERIAL** olarak okundu.
- Bu bulgular aynı DWG kaynağına göre birleştirilebilir. Yeni CI raporunda tek bir çizimin sınıf kaydı başka çizimin OT505 hedefini kabul ettiremez. Sınıf kaydı çözülemeyen dosyalarda semantik bağlantı **reddedilir**.
- Yeni tanı sadece örnek sayıları ve doğrulama durumlarını aktarır; ham handle, koordinat, katman adı, TEXT, uygulama EED verisi veya DWG kaynak baytlarını açıklamaz.

## Henüz kanıtlanmayan CAD işlevleri

Bu test **malzeme atama semantiği**, gerçek malzeme verileri, OT80/OT57'nin katman içindeki kesin rollerini, BYLAYER/BYBLOCK renk kalıtımını, CMC renklerini, katman açık/kapalı, frozen/locked, ölçüm, blok, render eşdeğerliği, DWG yazımı veya Android normal açmayı kabul etmez.

OT80 ve OT57 türü ile ilgili sayısal kayıtlar, referans sırası nedeniyle *plotstyle* veya *linetype* diye otomatik atanmamalıdır. Son 2–4 terminal H bitinin anlamı çözülmeden bütün H akışı tamamlandı iddiası kurulamaz.

## Sonraki kabul kapıları

1. OT80/OT57 hedef nesnelerinin veri sözlüğü ve gerçek referans rolü için kaynak-içi kimlik/CRC dışındaki ek bağımsız semantik kanıt.
2. Katman CMC + görünürlük bitlerinin gerçek kaynak eşleşmesi; gerekiyorsa viewport durumu.
3. Aynı DWG için bağımsız görünürlük/renk render eşdeğerlik testi.
4. Başarılı Android/iOS/macOS/Windows çekirdek kapıları ve final APK değerlendirmesi ancak tüm işlevsel kapsam doğrulandıktan sonra.

**Birleştirme:** PR #720 açık kalır, main'e otomatik aktarım ve APK iddiası yoktur. LibreDWG yalnızca ayrı GitHub CI host oracle sürecindedir.


## 6 Kasım 2026 — özgün C11 katman hedef üçlüsü doğrulaması

**Saat dilimi:** Europe/Istanbul (UTC+03:00)  
**Yeni test kod commit'i:** 45d9b7dd0206ae2c2bccbcb27e247574d0c33100  
**Başarılı gerçek DWG kaynak testi:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42182888167  
**CI çıktısı:** musacad-real-layer-class-bridge.json  
**Bağımsız çekirdek:** core/native/src/musacad_dwg_ac1024_self_index.c

Önceki yalnızca toplulaştırılmış sayımların ötesine geçildi. Yeni özgün C11
musa_dwg_ac1024_layer_eed_tail_type_triplet_probe fonksiyonu, her bir gerçek
OT51 LAYER kaydında dört tam H belirtecini ve ilk H5/0 boş belirtecini kontrol
edip sonraki üç H hedefini kaynak CRC16/self-H ile yeniden doğruluyor:
OT80 (sabit PLACEHOLDER), OT505 (bu kaynakta DXF MATERIAL olarak ayrı
Classes bölümünden çözümlenmiş dinamik sınıf) ve OT57 (sabit LTYPE).

**Gerçek kaynak sonucu:**
- **14/14** gerçek katman kaydı, aynı kaynak içindeki C11 üçlü tip kanıtından geçti.
- **42/42** hedef nesnesi, CRC / indeks / kendi H kimliği ve ham OT eşleşmesinden geçti.
- **10** kayıtta 2 bit ve **4** kayıtta 4 bit bitmemiş H-kuyruk parçası kaldı.
- Son parçaların **12**'si sıfır, **2**'si sıfır değildi; sıfır olanlara dolgu denmiyor.
- Python kaynak matrisi ayrıca C11 üçlü kabul sayısını ve kayıtlardaki terminal
  bit sayısını çapraz kontrol ediyor. Negatif sentetik testler tutarsız
  toplamları, kayıp hedefleri, kaynak çakışmasını ve eksik parçaları reddediyor.

**KESİN SINIR:** Bu bir veri kimliği ve ham H dizisi kanıtıdır. PLACEHOLDER,
LTYPE ve MATERIAL isimleri, söz konusu H sırasının nihai CAD rolünü,
malzeme atanmasını, çizgi tipi kalıtımını, CMC/BYLAYER rengini,
layer açık/kapalı/donmuş durumunu, geometri ya da render eşdeğerliğini
**kanıtlamaz**. C11 sonucunda h_tail_fully_decoded=0,
target_roles_verified=0 ve display_accepted=0 korunur. Tam son H
parçası çözülmeden hiçbir katman sahneye alınmaz.

LibreDWG yalnızca CI'daki ayrı host referans aracıdır; uygulamanın
özgün çekirdeğine eklenmedi. Kaynak DWG'ler değişmedi, PR #720
main'e birleştirilmedi ve APK üretilmedi.



## 6 Kasım 2026 — kaynak bit-deseni ve bayt-hizası incelemesi

**Saat dilimi:** Europe/Istanbul (UTC+03:00)  
**Kaynak kanıtın test edildiği commit:** 473147930e989ff6d0958abe35bae9f43e6aa93f  
**Başarılı gerçek kaynak Actions çalışması:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42185003836  
**Teknik çıktı:** musacad-real-layer-class-bridge.json

Özgün birinci taraf C11 çözümleyicisine kısa OT51 H-terminal parçasının
1..7 bitlik ham değeri ve bit-başı/bit-sonu mod 8 hizası eklendi.
Dosya adı, katman adı, koordinatlar, handle'lar veya DWG kaynak baytları
CI raporuna alınmıyor. Python sözleşmesi bit maskelerini örnek sayılarıyla
karşılaştırıyor; imkânsız değerleri ve sahte örnek kapsamasını reddediyor.

Kaynak DWG örnekleri üzerinde:
- **5 kaynak çizim**, **14 doğrulanmış OT51 kaydı** ve **42 CRC/self-H destekli hedef**.
- **10** parçanın uzunluğu **2 bit**, **4** parçanın uzunluğu **4 bit**.
- **12** parçada ham değer **0**, **2** parçada **2-bit değer 3 (binary 11)**.
- 2-bitliklerde görülen benzersiz ham değerler **{0,3}**;
  4-bitliklerde görülen benzersiz ham değerler **{0}**.
- Başlangıç hizası maskesi **80 (bit konumu 4 veya 6)**;
  bitiş hizası maskesi **1 (konum 0)**. Tüm 14 parça bayt sınırında bitiyor.
- Hiçbir parça henüz güvenilir biçimde padding sayılmadı. Sıfır olmayan
  2-bit alanlar özellikle kesin dolgu varsayımını engelliyor.
  h_tail_fully_decoded=false, rendering_verified=false ve
  layer_visibility_parity_proven=false korunuyor.

**Biçimle uyumlu, henüz işlev kabulü olmayan yorum:** ODA Open Design
Specification for .dwg files 5.1, madde 18.1.51 LAYER (sayfa 152),
R2000+ plotstyle/PLACEHOLDER; R2007+ material; common linetype
H referanslarını bu sırayla tarif ediyor. Bu biçim sırası
OT80 PLACEHOLDER → yerel OT505 MATERIAL → OT57 LTYPE üçlüsüyle uyumlu.
Belge: https://kldp.org/files/OpenDesign_Specification_for_.dwg_files.pdf
Buradaki biçim uyumu, gerçek malzeme/plotstyle/linetype kalıtımı,
görüntü renkleri veya AutoCAD eşdeğerliğini tek başına kanıtlamaz.

**Yeni regresyon koruması:** CI iş akışında en az **14 özgün C11 üçlü
kanıtı ve 5 kaynak DWG** aranıyor. Daha az kanıt üretildiğinde, yanlışlıkla
yeşil rapor verilmesi yerine CI başarısız olacak.
Bu yeni asgari eşik için son commitin test sonucu, önceki
4731479 kaynak çalışmasından ayrı izlenmelidir.
