# MusaCAD — önce 2B CAD, ardından 3B

**Karar (2026-10-16):** Ortak C/C++ CAD çekirdeği Android, Windows ve iOS için tek kaynak olacak. Mevcut 3B geliştirmeleri silinmeyecek; 3B ile ilgili yeni genişletmeler, 2B çekirdek kabulü tamamlanıncaya kadar ikinci öncelikte tutulacak. Güvenlik ve regresyon düzeltmeleri istisnadır.

## Kanıta dayalı başlangıç

Kaynak: `docs/AUTOCAD_2026_PARITY_MATRIX.json` ve `docs/AUTOCAD_2026_ACCEPTANCE_51.json`.

- Ön envanterde dosya, çizim, açıklama, blok ve doküman başlıklarıyla ilişkili **28 aile** var: **19 partial, 2 blocked, 7 unassessed**; bunlar henüz tamamlandı anlamına gelmez.
- Bu 28 aile **AutoCAD 2026'nın tüm 2B komutları değildir**. Her ailenin komutları, seçenekleri, nesne türleri, sınır durumları ve kaynak çizim senaryoları ayrıca açılmalıdır.
- Bağımsız tam AutoCAD 2026 eşdeğerlik onayı: **0/51**. CI yeşil olması, referans davranış ve gerçek cihaz eşdeğerliği değildir.
- README'de ve CI testlerinde 2B DXF okuma, LINE/LWPOLYLINE/CIRCLE/ARC, blok yerleştirme, yakınlaştırma, seçme, snap, OFFSET varyantları, katman/renk, metin, ölçü, PDF/yazdırma ve kurtarma için kod/örnekler mevcut. Bunların hiçbiri tüm seçenekleriyle bağımsız kabul edilmiş sayılmaz.
- Ortak CAD çekirdeği Android'e gömülü kalmamalı; platforma özgü UI, dosya seçici, yazdırma ve lisans işlevleri ayrı adaptörlerle sağlanmalı.

## Öncelik ve gerekli 2B kabul grupları

| Sıra | Grup | Mevcut durum / eksik kabul odağı |
| --- | --- | --- |
| P0 | Çizim veri modeli ve dosya güvenliği | Gerçek DWG/DXF read/edit/save/reopen; handle/owner, geometri, katman, renk, linetype, text ve BLOCK bilgilerinin kayıpsızlığı. Modern DWG yazma halen **blocked**. |
| P1 | Temel geometri | LINE, XLINE, RAY, PLINE/LWPOLYLINE, RECTANG, POLYGON, CIRCLE, ARC, ELLIPSE, SPLINE, POINT, DONUT, SOLID, TRACE; kesin koordinat, Z/OCS ve tüm komut seçenekleri. |
| P2 | Modifikasyon | MOVE, COPY, ROTATE, SCALE, MIRROR, OFFSET, STRETCH, LENGTHEN, ALIGN, ARRAY, TRIM, EXTEND, BREAK, JOIN, PEDIT, FILLET, CHAMFER, EXPLODE, grips; undo/redo ve gerçek kaydetme. |
| P3 | Hassas çizim ve seçim | OSNAP tüm kipler, OTRACK, ORTHO, POLAR, GRID, SNAP, dinamik giriş, WCS/UCS 2B, pencere/crossing/fence seçimi, layer/properties. |
| P4 | Görünüş ve tarama | HATCH (ada, pattern, gradient, associativity), linetype, lineweight, transparency, draw order ve büyük çizimde doğru ilk kare. |
| P5 | Yazılar, ölçülendirme, tablolar | TEXT/MTEXT, SHX/TTF, FIELD, DIM/DIMSTYLE, LEADER/MLEADER, anotatif ölçek, TABLE, tolerans/semboller ve geri yazma. |
| P6 | Bloklar ve dış kaynaklar | BLOCK/INSERT/ATTRIB/ATTDEF, nested transforms, MINSERT, dinamik bloklar, XREF (attach/reload/bind, çevrimler), görünürlük/özellik koruması. |
| P7 | Düzenler, çıktı ve ek belgeler | MODEL/PAPER, VIEWPORT, PLOT/PDF, ölçek/renk/kalem uyumu, sayfa setleri, PDFIMPORT, raster/underlay, XREF bağımlılıkları. |
| P8 | Çalışma verimliliği ve son 2B kabul | Büyük ve gerçek DWG dosyaları, bellek/latans regresyonu, RECOVER/AUDIT, çoklu belge, yakınlaştırmada ilk renk tutarlılığı, kayıpsız undo/redo. |

Her grup üç ayrı sonucu izler: **(1) kodda var**, **(2) otomatik test geçti**, **(3) aynı girdiyle bağımsız AutoCAD 2026 ve desteklenen platformlarda kabul edildi**. İlk iki durum üçüncünün yerine geçmez.

## İş akışı

1. Var olan kodları yeniden yazmadan, her 2B komutun oluşturma/görüntüleme/düzenleme/DWG-DXF kaydetme/yeniden okuma durumunu alt özellik bazında çıkar.
2. Başarısız, eksik ve yetersiz kapsama sahip işlevler için tekrar üretilebilir önce başarısız test, sonra **ortak çekirdek** düzeltmesi ekle.
3. Her özellik için kaynak çizim değişmezliği, fail-closed, precision, geometri/tür/handle/katman/renk korunması, büyük dosya ve undo/redo testleri ekle.
4. Derlemeleri güvenli tut; çalışan 3B modülleri ve Android sürümünü bozacak API değişikliği yapma. Yeni 3B yeteneklerine odaklanma; mevcut 3B regresyonlarını CI'da koru.
5. 2B komut/opsiyon kataloğu ve tüm bağımsız kabul kanıtları tamamlanmadan **2B %100** veya **AutoCAD eşdeğeri** deme. Her gelişme sonrası ölçülebilen tamamlanmış/kalan alt testleri raporla.
6. 2B aşamasının çıkışında 3B özelliği genişletmeye geç; B-Rep/ACIS, 3D çizim, yüzey, katı, Boolean ve render birikimini aynı ortak çekirdekte devam ettir.

**Dağıtım sınırı:** Kullanıcının ayrıca açık son onayı olmadan APK oluşturma, release veya canlı dağıtım **yapma**. CI ve host regression testi çalıştırılabilir. Bu yol haritası tek başına uygulanmış işlev veya tam eşdeğerlik iddiası değildir.
