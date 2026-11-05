# MusaCAD — AC1024/R2010 Classes sayısal ön kanıtı

**Tarih:** 2026-11-05  
**PR:** https://github.com/santiye2019-lab/MusaCAD/pull/720  
**Kod commit'leri:** `837136a` (bounded preamble), `8d67508` (numeric class records)  
**Durum:** İlk-taraf C11 ve sentetik örneklerde **yapısal kanıt**; gerçek `AcDb:Classes` bölümünden sınıf adı çözümü **YOK**.

## Bağımsız çekirdekte tamamlanan dar adım

- `core/native/src/musacad_dwg_ac1024_classes.c`: kullanıcı tarafından verilen, önceden açılmış mantıksal R2010 Classes aralığında açılış/kapanış sentinel işaretleri, RL/koşullu ikinci RL uzunluğu, BS maksimum sınıf, iki RC=0 ve B=1 ön koşulu sınır kontrollü incelenir.
- Bu alan içindeki sayısal sınıf kayıtları `BS classnum`, `BS proxy_flags`, `B was_proxy`, `BS item_class_id`, `BL instance_count`, `BS dwg_version`, `BS maintenance_version`, iki `BL` belirsiz veri olarak okunabilir.
- Numaralar artan ve tekil olmalı; sınıf kaydı türü 0x1F2 (entity) veya 0x1F3 (object) olmalı. Hatalı/eksik/çakışan kaynakta çıktı değiştirilmez.
- Sınıf **numarası 505** için üretilen sentetik kayıt iki boyut varyantında okunur. Bu test *gerçek Karagümrük OT505'in adını veya türünü çözmez*.
- Ortak CMake C11 hedefinde çalışır; `musacad_core_ac1024_classes_prefix_shape_smoke` CTest'e bağlandı. [159/159 test başarılı](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42083495300).

## Gerçek projelerden ayrı olarak daha önce kanıtlananlar

- [OT51 konumsal ham tür kanıtında](https://github.com/santiye2019-lab/MusaCAD/blob/feat/libredwg-host-oracle-pilot-720/Raporlar/AC1024_OT51_SIRA_BAZLI_HAM_TUR_KANITI_2026-11-05.md) beş Karagümrük AC1024 dosyasından **14** LAYER örneği; tam 42 hedef CRC/self-H eşleşmesi; sırasıyla OT80, OT505, OT57 ham türleri bulundu.
- Yeni [gerçek DWG kaynak matrisi](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42083495318) bu yeni kodla derlenip çalışmıştır. Ancak **R2010 AcDb:Classes bölümünü özgün sayfa haritasından doğrulayıp açma veya OT505 adını bulma aşamasını yapmamaktadır.**

## Kabul edilmeyen iddialar ve sınırlar

1. **Sınıf 505 = MATERIAL, plotstyle, XREF, görünürlük veya başka bir rol** sonucuna ulaşılamaz. Ham OT505 çizimin yerel dinamik sınıf numarasıdır; adının kaynağı `Classes` içindeki Unicode string akışıdır.
2. Bu yeni modülde `original_section_authenticated=0`, `class_name_resolved=0`, `display_accepted=0` zorunlu olarak kalır; alana ait CRC/korunmuş fiziksel sayfalar doğrulanmaz.
3. R2010 bakım sürümüne göre olası ek RL varyantı çağıranın *açık seçimiyle* denenir; biçimden tahmini sürüm çıkarılmaz. CTest gerçek DWG uyumluluk/eşdeğerlik testi değildir.
4. Orijinal DWG kaynağı yazılmadı, LibreDWG çalışma zamanı kütüphanesine eklenmedi, PR ana dala birleştirilmedi ve APK üretilmedi.

## Sonraki teknik kabul

**Önce** fiziksel sayfaları/harita özetlerini özgün AC1024 imzası ile doğrulayan Classes çıkarıcısı; **sonra** özgün Unicode TU isim akışı ve classnum 505 kimliği; **en son** katman/renk/görünürlük/linetype gerçek sahne kanıtı. Kaynağa dayanmayan bağlantı ve çizimler reddedilmelidir.

**Biçim referansı:** [Open Design Specification for .dwg files, §10.2 R18+](https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf) — yalnızca biçim şeması; kitaplık kodu kullanılmamıştır.
