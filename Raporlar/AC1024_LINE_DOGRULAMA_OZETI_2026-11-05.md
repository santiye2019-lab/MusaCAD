# MusaCAD — AC1024 LINE kaynak-içi doğrulama özeti

**Tarih:** 2026-11-05  
**Durum:** Salt-okunur mühendislik tanı kanıtı; çizime kabul değil  
**Çalışma:** [PR #720](https://github.com/santiye2019-lab/MusaCAD/pull/720)  
**Son gerçek DWG testi:** [GitHub Actions #41978534284](https://github.com/santiye2019-lab/MusaCAD/actions/runs/41978534284)  
**EED APPID testi:** [GitHub Actions #41913863728](https://github.com/santiye2019-lab/MusaCAD/actions/runs/41913863728)

## Örnekleme ve doğrulama

Karagümrük proje grubundaki 8 AC1024/R2010 çiziminin **5.142 örnek LINE kaydı** salt-okunur olarak tarandı. Bu rapor yalnızca toplam sayıları içerir; proje bazında dosya adı, yol, çizim koordinatı veya ekran görüntüsü vermez.

| Kanıt sınıfı | Sayı |
| --- | ---: |
| Örneklenen LINE | 5.142 |
| Koordinatları çözümlenen | **5.141** |
| Kaynak-içi doğrulama oranı | **%99,98** |
| Açık sahip + katman + LINE XYZ birleşik kanıtı (sıfır EED, olağan H) | 2.801 |
| Örtük alan katmanı + LINE XYZ birleşik kanıtı (sıfır EED, olağan H) | 1.313 |
| XDictionary H + katman ve gerektiğinde sahip + LINE XYZ kanıtı (sıfır EED) | 251 |
| EED + LINE XYZ + özgün katman/varsa sahip kanıtı | 776 |
| EED kayıtları içinde açık BLOCK_HEADER sahipliği | 665 |
| EED kayıtları içinde örtük alan (sahip H yok) | 111 |
| EED kayıtları içindeki doğrulanan APPID (OT67) H referansı | **783** |
| APPID referansı doğrulanan EED LINE | **776** |
| APPID ref. doğrulama reddi | 0 |
| Sıfır çizgi tipi ölçeği nedeniyle reddedilen LINE | **1** |

Birbirinden ayrı dört kanıt sınıfı toplamı `2.801 + 1.313 + 251 + 776 = 5.141` kayıttır; bu sınıflar toplanırken hiçbir kaydın iki kez sayılmaması gerekir. EED içindeki 776 LINE, toplam 783 APPID referansı taşır. 111 örtük alan EED kaydı için bir `BLOCK_HEADER` sahibi **üretilmemiştir**.

## Kalan kayıt

Tek kalan LINE, CRC ve ön alanları geçse de kodlanmış `LTSCALE` değeri **0** olduğu için reddedildi. Tanılama, bunun negatif ölçek ya da çözülemeyen BD değerinden kaynaklanmadığını belirledi. Kabul kuralı gevşetilmedi; `0` otomatik olarak `1` yapılmadı.

## Kanıt sınırları

- Bunlar özgün kaynakta CRC, tür, kimlik, referans ve veri sınırı kanıtlarıdır; bağımsız AutoCAD görüntü/geometri eşdeğerliği değildir.
- Katman **görünürlüğü, dondurma/kilitleme**, renk kalıtımı, blok INSERT dönüşümleri, XREF ve ekran sahnesine kabul **henüz doğrulanmamıştır**.
- Otomatik DWG açma, yazma veya uygulama üzerinden eksiksiz 2B görüntüleme bu raporla kanıtlanmaz.
- LibreDWG yalnızca izole host oracle doğrulama iş akışındadır; MusaCAD ortak çekirdek içinde kullanılmaz.
- Kaynak DWG dosyaları değiştirilmedi. Ana dala birleştirme ve APK oluşturma yapılmadı.
- Bu herkese açık depoda yalnızca toplu mühendislik metrikleri kayıt altına alınır. Kullanıcıya/cihaza özgü ayrıntılı raporların otomatik yüklemesi etkin değildir.

## Sıradaki bağımsız doğrulama

1. `LAYER (OT51)` nesnesinin görünürlük, donmuş/kilitli ve renk koşullarını kaynak-içi kanıta bağlamak.
2. `BLOCK_HEADER (OT49)`, `INSERT (OT7)` ilişkilerini ve iç içe dönüşümleri model/kâğıt alanı sınırlarıyla doğrulamak.
3. ARC, CIRCLE ve diğer 2B nesneleri ayrı kaynak kanıtı ve negatif testlerle kapsamak.
