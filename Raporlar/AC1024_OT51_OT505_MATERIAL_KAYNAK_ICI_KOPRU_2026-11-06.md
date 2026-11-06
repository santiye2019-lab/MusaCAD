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
