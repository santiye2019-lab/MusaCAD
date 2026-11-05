# MusaCAD — AC1024 OT51 LAYER ham bayrak ve EED APPID kanıtı

**Tarih:** 2026-11-05  
**Kapsam:** Salt-okunur, gerçek DWG kaynak-içi tanı; tam DWG görüntüleme veya AutoCAD eşdeğerliği değildir.  
**Geliştirme:** [PR #720](https://github.com/santiye2019-lab/MusaCAD/pull/720)  
**Katman ham bayrak testi:** [GitHub Actions #41987476727](https://github.com/santiye2019-lab/MusaCAD/actions/runs/41987476727)  
**LAYER EED APPID testi:** [GitHub Actions #41988280548](https://github.com/santiye2019-lab/MusaCAD/actions/runs/41988280548)

## Gerçek kaynak kanıtı — Karagümrük çizim örneklemi

Aşağıdaki toplamlar kaynak-içi tanı sırasında seçilmiş **15 özgün OT51 LAYER** kaydına aittir. Bütün DWG dosyalarındaki tüm katmanların eksiksiz envanteri olduğunu iddia etmez.

| Kanıt | Doğrulanan |
| --- | ---: |
| Örneklenen özgün OT51 LAYER | 15 |
| Sıfır EED içeren kaydın CRC/sayısal-bayrak kanıtı | 1 / 1 |
| Sıfır olmayan EED içeren kaydın CRC/sayısal-bayrak kanıtı | 14 / 14 |
| Toplam özgün ham katman-bayrağı kanıtı | **15 / 15** |
| Sıfır olmayan EED'li OT51'in APPID (OT67) hedef kanıtı | **14 / 14** |
| EED'li OT51 içindeki toplam orijinal APPID H5 referansı | **24** |
| EED'li OT51 APPID hedefi doğrulama reddi | **0** |
| EED'li OT51 üzerinde XDictionary-var biti | **14** |

EED içermeyen tek katmanın EED'ye özel okuyucuda reddedilmesi **beklenen ayrım**dır; iki yolun kabul kümeleri ayrı tutulur. 14 EED'li katmanın tümü, orijinal uygulama H5 referanslarını **CRC doğrulanmış, OT67, kaynak self-H kimliği** ile ilişkilendirmiştir. Kaynak dosyalar değiştirilmemiş, uygulama EED baytları yorumlanmamıştır.

## Henüz kanıtlanmayanlar

- **14 XDictionary hedefi (OT42) doğrulanmadı**: yalnızca özgün XDictionary-var biti tespit edildi; özgün H target role ve nesne zinciri ayrı sınanmalı.
- Ham OT51 bayrakları gerçek viewport/model/kâğıt alanı **görünürlüğünü, çizdirme davranışını veya donmuş katman semantiğini kanıtlamaz**. Çizim için `display_accepted = 0` korunur.
- LAYER CMC/BYLAYER renk, renk kalıtımı, çizgi tipleri, katman isimleri, INSERT/blok dönüşümleri, XREF ve sahne kabulü henüz bu kanıtla sağlanmaz.
- Bu rapor yalnızca toplu metrikleri içerir; orijinal çizim yolları, koordinatlar, kullanıcı bilgisi veya uygulama özel EED baytları bulunmaz.

## Takip sırası

1. AC1024 özgün LAYER OT51 XDictionary H bağlantısını ve OT42 hedeflerini kaynak-kayıt kanıtıyla doğrulamak.
2. LAYER isim, CMC renk, viewport/override ve XREF görünürlük alanlarını bağımsız sıkı testlere ayırmak.
3. Kaynak-içi doğrulamadan sonra ayrı, bağımsız görüntü eşdeğerliği testi yapmak; onaydan önce nesneleri sahneye almamak.

**Ana dala birleştirme ve final APK:** Bu rapor kapsamında yapılmadı.
