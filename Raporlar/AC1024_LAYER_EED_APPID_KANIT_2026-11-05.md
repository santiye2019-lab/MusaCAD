# MusaCAD — AC1024 OT51 LAYER ham bayrak ve EED APPID kanıtı

**Tarih:** 2026-11-05  
**Kapsam:** Salt-okunur, gerçek DWG kaynak-içi tanı; tam DWG görüntüleme veya AutoCAD eşdeğerliği değildir.  
**Geliştirme:** [PR #720](https://github.com/santiye2019-lab/MusaCAD/pull/720)  
**Katman ham bayrak testi:** [GitHub Actions #41987476727](https://github.com/santiye2019-lab/MusaCAD/actions/runs/41987476727)  
**LAYER EED APPID testi:** [GitHub Actions #41988280548](https://github.com/santiye2019-lab/MusaCAD/actions/runs/41988280548)  
**Ham ilk H kodu (H4/1 bayt):** [GitHub Actions #42011036445](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42011036445)  
**OT50 + OT42 özgün H-graph testi:** [GitHub Actions #42011969838](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42011969838)

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
| Özgün OT51 LAYER_CONTROL H4 (OT50) + XDictionary H3 (OT42) hedef kanıtı | **14 / 14** |
| H4+H3 birleşik grafikten reddedilen | **0** |
| H4+H3 ardından ek çözümlenmemiş handle bitleri olan kayıtlar | **14** |

EED içermeyen tek katmanın EED'ye özel okuyucuda reddedilmesi **beklenen ayrım**dır; iki yolun kabul kümeleri ayrı tutulur. 14 EED'li katmanın tümü, orijinal uygulama H5 referanslarını **CRC doğrulanmış, OT67, kaynak self-H kimliği** ile ilişkilendirmiştir. Kaynak dosyalar değiştirilmemiş, uygulama EED baytları yorumlanmamıştır.

## Doğrulanan özgün XDictionary bağlantısı

İlk deneme 14/14 reddedildi; salt-okunur aşama tanısı, reddin **ilk LAYER_CONTROL handle'ında** oluştuğunu belirledi. DWG biçim tanımında bu ilişki bir **soft pointer (H4)** olmasına karşın ilk C okuyucusu yanlışlıkla H2 kodunu bekliyordu. Gerçek kaynakta **14/14 H4, uzunluk 1 bayt** gözlemlendi. Kod H4 olacak şekilde düzeltildi; OT50 LAYER_CONTROL ile OT42 DICTIONARY hedefleri ayrıca özgün CRC, tip ve self-H kontrolünden geçti (**14/14**). Yanlış H2 rolü, bozuk kaynak CRC ve geçersiz hedefler negatif sentetik testlerde reddediliyor.

## Henüz kanıtlanmayanlar

- 14/14 grafikte **OT50 + OT42 sonrası başka handle alanları** vardır; bunların türleri, bağları, sıraları veya semantiği bu aşamada doğrulanmamıştır.
- Ham OT51 bayrakları gerçek viewport/model/kâğıt alanı **görünürlüğünü, çizdirme davranışını veya donmuş katman semantiğini kanıtlamaz**. Çizim için `display_accepted = 0` korunur.
- LAYER CMC/BYLAYER renk, renk kalıtımı, çizgi tipleri, katman isimleri, INSERT/blok dönüşümleri, XREF ve sahne kabulü henüz bu kanıtla sağlanmaz.
- Bu rapor yalnızca toplu metrikleri içerir; orijinal çizim yolları, koordinatlar, kullanıcı bilgisi veya uygulama özel EED baytları bulunmaz.

## Takip sırası

1. OT51 LAYER için **kalan handle alanlarını** rol ve kaynak CRC kanıtıyla sınamak; XREF, plotstyle, malzeme ve çizgi tipini varsayılan olarak kabul etmemek.
2. LAYER isim, CMC renk, viewport/override ve XREF görünürlük alanlarını bağımsız sıkı testlere ayırmak.
3. Kaynak-içi doğrulamadan sonra ayrı, bağımsız görüntü eşdeğerliği testi yapmak; onaydan önce nesneleri sahneye almamak.

**Ana dala birleştirme ve final APK:** Bu rapor kapsamında yapılmadı.


## R2010 OT51 remaining H-token shape preflight — 2026-11-05

Source-only evidence at CI [42014832418](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42014832418): among **14** tested original EED-bearing OT51 layer records across five Karagümrük drawings, **14/14** CRC-backed original OT50/H4 and OT42/H3 target chains passed. The next raw H header had code **5** and **zero payload bytes** in **14/14**, but this does **not** assign linetype, plotstyle, material, viewport or color semantics.

A new strictly bounded **shape-only** scan now counts all remaining H token headers and their declared payload octets after the two proven references, with 64-token cap, exact bit termination and original source CRC validation. It intentionally refuses malformed, excessive or incomplete spans. Synthetic and real-DWG CI for this NEW scan are pending; no semantic or rendering parity is claimed. Production `display_accepted=0` is unchanged.
