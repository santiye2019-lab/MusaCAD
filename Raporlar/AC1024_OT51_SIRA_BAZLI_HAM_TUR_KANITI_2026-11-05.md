# MusaCAD — AC1024/R2010 OT51 LAYER handle sırası ve ham OT hedef kanıtı

**Tarih:** 2026-11-05  
**Durum:** Doğrulanmış salt-okunur, kaynak-içi sayım; **görsel eşdeğerlik ve katman semantiği kabulü değildir.**  
**İlgili PR:** [#720](https://github.com/santiye2019-lab/MusaCAD/pull/720)  
**Test edilen çekirdek commit:** [`08e8f2d`](https://github.com/santiye2019-lab/MusaCAD/commit/08e8f2dd891ec0f1aa88c11548024246349b665b)  
**Gerçek kaynak testi:** [GitHub Actions #42077850141](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42077850141)  
**Salt sayım artefaktı:** [#13438518135](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42077850141/artifacts/13438518135)

## İncelenen kaynak

23 takip edilen DWG dosyası için gerçek kaynak/LFS doğrulama matrisi çalıştı; bu rapor yalnızca AC1024/R2010 içindeki ilk uygun **14 OT51 LAYER örneğinin** sonuçlarını kapsar. Çıktı tam evren sayımı, AutoCAD görüntüleme testi veya kullanıcı ekranındaki çizim doğruluğu değildir. Kaynak DWG dosyaları işlem öncesi ve sonrasında doğrulandı; **değiştirilmedi**.

Her örnek için özgün nesne CRC16'sı, self-H, doğrulanmış OT50 layer-control ve OT42 XDictionary bağlantıları, EED APPID önkoşulu ve dört tamamlanmış ham H başlığı kaynak-içi olarak yeniden denetlendi. Son 2/4 bit, anlamı bilinmediğinden birleştirme/görüntüleme için **reddedilmiş** bırakıldı.

## Somut ölçüm

| Ham H kuyruk konumu | Sayısal hedef gözlemi | Doğrulanmış hedef ham OT | Örneklem |
| --- | --- | --- | ---: |
| 1 | H kodu 5, sıfır bayt: mutlak hedef yok | — | 14/14 |
| 2 | Tam mutlak H; özgün self-H ve CRC ile eşleşti | **80** | 14/14 |
| 3 | Tam mutlak H; özgün self-H ve CRC ile eşleşti | **505** | 14/14 |
| 4 | Tam mutlak H; özgün self-H ve CRC ile eşleşti | **57** | 14/14 |
| **Toplam** | 56 tamamlanmış H başlığı, 42 mutlak ve eşleşen aday, 14 hedefsiz | **42/42** mutlak hedef doğrulandı | **14 kayıt** |

- Mutlak aday/özgün hedef eşleşmesi: **42/42**; doğrulama reddi: **0**.
- Bilinen OT49 BLOCK_HEADER ya da OT51 LAYER sınıfına eşleşen bu 42 aday: **0**. Her üç diğer sınıfın sırası, bu küçük örneklemde tutarlı.
- Kaynakta kalan 2/4 bitlik son parçaların önceki tanısında **12 tamamen sıfır, 2 sıfır olmayan** örnek vardı. Sıfır görülen parça bile **dolgu kabul edilmez**.
- `sampled_layer_eed_terminal_candidate_ordinal_*_raw_ot_consensus` sayısında `0`=eşleşme yok, `65535`=karışık tür, diğer değerler **orijinal OT + 1**'dir. Bu örneklemde sırasıyla `0, 81, 506, 58` döndü; yani ham OT `80, 505, 57`.

## Doğrulanmayanlar

**OT80/OT505/OT57 için linetype, plotstyle, material, XREF veya başka bir CAD rolü atanmamıştır.** Özellikle OT505 gibi sınıf kimlikleri için sınıf tablosu ve referans semantiği bağımsız doğrulanmalıdır. R2010 DATA içindeki CMC renk, katmanın açık/kapalı/donmuş/kilitli oluşu, kalıtım, blok INSERT geometrisi, kağıt/model alanı üyeliği, görüntüleme, yazma ve AutoCAD 2027 eşdeğerliği bu testin kapsamı dışındadır.

LibreDWG yalnızca ayrı GitHub CI referans sürecinde çalışmıştır; ilk taraf ortak çekirdeğin içine bağlanmamıştır. Bu rapor kaynak handle değerleri, koordinatlar, çizim içeriği veya katman adları içermez. PR ana dala birleştirilmedi ve APK üretilmedi.

## Sıradaki kabul kapısı

1. Üç eşleşen hedef sınıfının ham OT ve, gerekiyorsa, dinamik CLASS kayıtlarını kendi ilk-taraf çekirdeğimizle doğrula; **sıra benzerliğinden rol çıkarma**.
2. Son 2–4 bitin R2010 handle akışı üzerindeki gerçek yapısal anlamını bağımsız pozitif/negatif kaynak ve sentetik testlerle kanıtla.
3. Ancak bundan sonra katman CMC ve görünürlük kurallarını, ayrıca LINE/INSERT blok ilişkilerini gerçek DWG sahnesiyle karşılaştır. Başarısız veya belirsiz sahne **fail-closed** kalmalıdır.
