# MusaCAD CAD Motoru — 18 Maddelik Tek Kabul Envanteri

> **Ana kayıt:** [GitHub #249](https://github.com/santiye2019-lab/MusaCAD/issues/249).  
> **Son envanter denetimi:** 2026-10-13.  
> **Karar:** **18/18 madde nihai kabul bekliyor.** Yeşil GitHub CI ≠ gerçek Android cihaz kabulü ≠ tam AutoCAD özellik eşdeğerliği.  
> Bu tablo, #249 kapsamını **değiştirmez**. Yeni bir özellik/eski hata bulunduğunda 18 numarayı yeniden başlamadan, mevcut maddenin alt kabul kriterine veya ayrı genişletme envanterine ekle.

## Değişmez kabul kuralları

- Bir maddeye **TAMAMLANDI** yazmak için: **gerçek dosya kanıtı**, otomatik testler, GitHub quality gate, gerçek Android telefon saha kabulü ve yazılı sonuç **birlikte** bulunmalıdır.
- Test başarısı iddiası yalnız test edilen branch/commit için geçerlidir. `main` veya kurulu APK'ya otomatik aktarılmış anlamına gelmez.
- **Okundu** ile **ekranda doğru çizildi** ve **düzenlenip kayıpsız kaydedildi** ayrı test edilir. Sessiz nesne atlaması başarı sayılmaz.
- Orijinal DWG/DXF SHA-256 değişmeyecek; hedefte yanlışlıkla üzerine yazma, kaynak kaybı, bozuk kayıt veya kilitlenme sıfır-tolerans hata.
- ACIS SAT'nin salt okunur tessellation'ı, native ACIS/SAB düzenleyicisi değildir. `3DFACE` ağ dışa aktarımı da düzenlenebilir `3DSOLID` değildir.
- Dürüst rapor: **Kod yok / kısmi kod / CI geçti / cihaz bekliyor / kabul edildi**. Belirsiz alanlar **doğrulanmadı** görünür; varsayımla yeşil işaret koyulmaz.
- Yeni kod yalnız feature branch üzerinde; mevcut üretim APK'sı, AI/Cloudflare/lisans servisi ve `main` korunur. Saha testinde başarısızlık varsa düzeltme + regresyon + yeniden CI döngüsü yapılır.

## Ana madde matrisi (1–18, #249 ile birebir)

| # | Orijinal kapsam | Kanıt / bağlı çalışma | Kod / CI aşaması | Kabul için eksik ana koşul |
|---|---|---|---|---|
| 01 | Salt 3D DWG/DXF açılışı; SAT/SAB | [#246](../pull/246), [#242](../issues/242) | Kısmi uygulama, #246 CI geçti | SAT/SAB tür bazlı sadakat, orijinal gerçek dosyalar ve telefon |
| 02 | 3D DXF güvenli kayıt ön-denetimi | [#247](../pull/247), [#244](../issues/244) | Kısmi uygulama, CI geçti | SAF sağlayıcı hatası/yarım çıktı ve telefon kayıt testi |
| 03 | 2D DXF edit→save→reopen roundtrip | [#253](../pull/253), [#244](../issues/244) | Koruyucu staging var, #253 CI geçti; kapsam tamamlanmadı | Birim, Unicode, HATCH, blok, OLE, DIMSTYLE, orijinal hash karşılaştırması |
| 04 | Düzenlenmiş gerçek DWG yazma | [#266](../pull/266), [#244](../issues/244) | **Host-only** deneysel R2000 writer CI geçti; Android arayüzü yok | Modern sürümler, koordinat/özellik eşdeğerliği, XREF/ACIS ve cihaz |
| 05 | 3D viewport culling, batching ve FPS | [#248](../pull/248), [#243](../issues/243) | Kısmi optimizasyon, CI geçti | Gerçek cihaz p50/p95 frame time, büyük mesh, ısınma/ANR |
| 06 | İlk açılış gerçek renk (TrueColor/ByLayer/ByBlock) | [#243](../issues/243) | **Tam kapsam doğrulanmadı** | İlk kare ekran görüntüsü; renk-parite fixture ve zoom öncesi test |
| 07 | Büyük dosya açılışı, pan/zoom, bellek | [#243](../issues/243) | **Tam kapsam doğrulanmadı** | Küçük/orta/büyük; 10 tekrar soğuk/sıcak p50/p95, RAM/ANR |
| 08 | Atlanan/unsupported nesnelerin açık sayımı | [#250](../pull/250) | Kısmi atlama envanteri, CI geçti | Kaynak object+handle+layer tam tarama, kaynak dosya ile eşleştirme |
| 09 | Çoklu proje sekmeleri, iptal/kurtarma | [#256](../pull/256) | Kısmi güvenli-kapatma, CI geçti | 4+ sekme, eşzamanlı hazırlama, bellek ve gerçek telefon |
| 10 | XREF/görsel referansı attach/reload/bind/nested | [#249](../issues/249) | **Eksiksiz uygulama doğrulanmadı** | Harici/eksik referans ve nested path çizim+save roundtrip |
| 11 | Dinamik blok & MINSERT | [#257](../pull/257), [#253](../pull/253) | OCS MINSERT ve ATTRIB kısmi, CI geçti | Parametre/visibility/action/grip, yeniden yazma ve metadata |
| 12 | Layout/viewport/plot ve VP freeze | [#249](../issues/249) | **Tam kapsam doğrulanmadı** | Sayfa boyutu, plot scale, görünür katman, fiziksel/PDF baskı |
| 13 | Associative DIMSTYLE/annotative/tolerans | [#249](../issues/249) | **Tam kapsam doğrulanmadı** | Stil, associativity, annotative scale, tolerans okuma/düzenleme/kaydetme |
| 14 | HATCH/SPLINE/MLINE/MTEXT/fontlar | [#249](../issues/249) | **Tam kapsam doğrulanmadı** | Eğri ve font geometri/parite, gerçek font ikamesi ve roundtrip |
| 15 | 2D MOVE/COPY/TRIM/EXTEND/ARRAY/FILLET/OFFSET/EXPLODE/UNDO/REDO | [#249](../issues/249) | **Fonksiyon matrisi kesinleştirilmedi** | Her komut: açık/kapalı sınır durumu, zincirli iş akışı, kayıt ve geri alma |
| 16 | OCS/UCS, Z kotu, 3D→2D | [#257](../pull/257), [#258](../pull/258)–[#265](../pull/265) | Aşamalı UCS/gizmo/kurtarma kısmi; #257 ve #265 CI geçti | Tüm OCS entity türleri, eğik koordinat, sayısal parite, telefon |
| 17 | Gerçek B-Rep/ACIS/SAB read-write | [#267](../pull/267) | **Yalnız polygonal B-Rep extrusion prototipi**; #267 CI geçti | Native exact solid; ACIS/SAB edit/save, metadata/topoloji doğrulaması |
| 18 | EXTRUDE/SWEEP/LOFT/REVOLVE/BOOLEAN/SECTION, 3D UCS/gizmo ve GPU | [#267](../pull/267), [#260](../pull/260) | Basit EXTRUDE polygon ağı ve UCS/gizmo kısmi, CI geçti | Kalan 3D komutlar, boolean geçerliliği, GPU render ve telefon |

**Hiçbir satır henüz “tamamlandı” değildir.** CI sonucu, daha önce listelenen baş PR commit'inin GitHub iş akışını ifade eder; çapraz PR birleşimini veya saha kabulünü doğrulamaz.

## Her maddenin kapanış dosyası

Her satır için aşağıdaki kanıt kutuları ayrı kapatılır:

- [ ] **F0 — Fixture:** Küçük, orta, büyük; bozuk/unsupported; beklenen tür/sayı/koordinat/renk/öznitelik manifesti
- [ ] **F1 — Implementasyon:** Değişiklikler, geri-alma planı, bağımlılık ve lisans sınırı belgeli
- [ ] **F2 — Otomatik test:** Okuma/yazma, kötü veri, iptal, orijinal hash, düzenle/kaydet/yeniden aç senaryosu
- [ ] **F3 — GitHub CI:** Test edilen SHA, başarısız adım ve düzeltme yinelemesi kayıtlı
- [ ] **F4 — Gerçek Android:** Aynı/sınanmış APK, gerçek cihaz, ekran döndürme, arka plana alma, RAM/ANR, pano/zoom
- [ ] **F5 — Kabul:** Sonuçlar kaynak dosyalarla karşılaştırılmış, açık regresyon yok; onay tarihi ve artefakt bağlantısı var

### Ortak sayılandırılmış ölçütler

- Orijinal dosyanın **önce/sonra SHA-256'sı eşit**; ayrı çıktı hatasız tam kapanmalı.
- En az 3 dosya boyu; her açılış/zoom-panoramada 10 tekrarlı **p50/p95**; anormal kilit/ANR ayrı etiketli.
- **İlk kare renk parity** (zoom dokunuşu olmadan), nesne sayımı, nesne türü, handle/layer, koordinat/snap/ölçü birimi, layout, metraj ve çıktı doğruluğu.
- Gerçek cihazın **üretici/modeli**, Android sürümü, APK commit/hash, test çizim hash'leri ve gözlenen hata raporda yer almalı.
- Uygulamadaki Gandalf AI ve mühendislik disiplin analizleri, **CAD çekirdeği kabulünden ayrı ileri seviye üstünlük ölçütleridir**. AutoCAD'e karşı üstünlük ancak performans/fonksiyon karşılaştırmalı testlerle iddia edilir.

## Birleştirme ve yayın bağımlılıkları

`#247 → #258 → #259 → #260 → #261 → #262 → #263 → #264 → #265 → #266 → #267` üst üste geliştirme dalı zinciridir; ayrıca #246, #248, #250, #253, #256, #257 paralel taslaklardır. **Bunları birbirinden bağımsız biçimde main'e merge etmek doğru değildir**: aynı önceki commit'ler bu PR'lerde de bulunur.

**Son APK kapısı:** 18/18 kapsam kabulü (ya da açıkça müzakere edilmiş daha dar sürüm kapsamı), tüm ilgili CI ve gerçek cihaz kabulü, lisans/AI regresyonu, üretim APK build onayı, artefaktın/hash'in indirme kanıtı. **Bu koşullar sağlanmadan “tüm araçlar tamam” ya da “AutoCAD'den üstün” yazılmaz.**

## Başvuru ve karar günlüğü

- 2026-10-13: Ana kaynak #249 yeniden okundu; ilgili açık PR'ler karşılaştırıldı; taslak/CI ≠ cihaz kabulü ilkesi teyit edildi.
- 2026-10-13: #266 R2000 host-only gerçek DWG yazma denemesi ve #267 polygonal B-Rep extrusion önceki CI çalışmaları yeşil; her ikisi de nihai motor kapsamının yalnız bir bölümünü kapsıyor.
- Yeni tespitler mutlaka ilgili **numaralı satıra** ve ayrı PR'ye bağlanacak. Kabul edilmeyen madde geriye dönmüş sayılmaz; hiçbir satır sahte tamamlanmış durumuna getirilmeyecek.
