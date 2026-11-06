# MusaCAD — Gerçek R2010 LINE → OT51 ham katman durumu / renk indeksi kanıtı

**Tarih:** 6 Kasım 2026, Türkiye saati (UTC+03:00)  
**PR:** https://github.com/santiye2019-lab/MusaCAD/pull/720  
**C11 kod commit'i:** 098457dd048c3263393643be59824d3990c1743c  
**Başarılı gerçek DWG kaynak testi:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42219353489  
**CI artifact:** musacad-real-dwg-native-stage-diagnostic / musacad-real-native-stages.json / musacad-real-layer-class-bridge.json

## Uygulanan bağımsız çekirdek denetimi

Yeni özgün C11 yordamı musa_dwg_ac1024_line_eed_raw_layer_color_join, **aynı fiziksel AcDbObjects kaynak bölümü ve aynı kurtarılmış self-H indeksi** üzerinden:

1. EED taşıyan sabit OT19 LINE kaydının kaynak CRC/self-H, çizgi koordinat sınırı, EED ve H-graph kanıtını yeniden kontrol eder.
2. LINE içindeki orijinal layer H'nin işaret ettiği CRC/self-H doğrulamalı OT51 LAYER kayıt kimliğini zorunlu kılar.
3. Aynı OT51 için raw frozen/on/locked bitlerini, OT80/çizime özel OT505/OT57 H hedeflerini ve henüz çözümlenmemiş terminal bayt hizasını ayrı denetler.
4. LINE common CMC alanından **ham renk indeksini** okur. İndeks 256 sayımı BYLAYER adayıdır; katman CMC rengi veya piksel eşdeğerliği değildir.
5. İspat eksikse fail-atomic ret verir. Kaynak DWG dosyasını, handle değerlerini, koordinatları, katman isimlerini ve gerçek çizim metinlerini rapora çıkarmaz.

## Gerçek kaynak sonuçları

| Kanıt | Sonuç |
| --- | ---: |
| Tüm kaynak dosyalar | 23 DWG |
| AC1024 kaynak dosyalar | 10 DWG |
| Önceden geometri + H-graph kanıtı olan EED LINE | 776 |
| Bu yeni dar test için örneklenen LINE sayısı | **297** |
| Aynı kaynakta LINE → LAYER ham durum + renk indeksi doğrulanan | **214** |
| Güvenli biçimde reddedilen örnek | **83** |
| Kabul edilmiş 214 kayıt içinde ham CMC indeksi 256 | **128** |
| Kabul edilenler içinde LINE üzerinde ayrıca açık RGB kaydı görülen | **0** |
| Kabul edilenlerde frozen=1 | **0** |
| Kabul edilenlerde on=1 | **0** |
| Kabul edilenlerde locked=1 | **0** |

**Kapsam notu:** Kaynak başına ilk en çok 64 daha önce EED LINE/H graph kanıtı doğrulanmış kayıt seçildi. Dolayısıyla 297/214/83 ve 128 sayıları tüm çizim nesnelerinin tam envanteri değildir. Reddedilen 83 kaydın hangi tekil ön koşul nedeniyle reddedildiği bu raporda sınıflandırılmadı; retleri eksik veya hatalı DWG diye tanımlamayın.

## Asla iddia edilmeyecekler

- Renk indeksinin 256 olması **gerçek BYLAYER renk aktarımının doğru olduğunu** kanıtlamaz. OT51'in CMC kaynağı ve renk yönteminin ayrıca çözülmesi gerekir.
- frozen/on/locked ham bitleri çizim görünürlüğü, kilitli düzenleme, viewport gizlemesi veya blok içi nihai görünürlüğü tek başına vermez.
- LINE koordinat/CRC/H kanıtı, otomatik WCS/eşdeğer çizim, ekran piksel testi veya DWG yazma/AutoCAD eşdeğerliği değildir.
- Dinamik OT505'in yerel MATERIAL sınıf kimliği, malzeme özelliklerinin render etkisi olarak kabul edilmez.
- C11 çıkışındaki bylayer_color_parity_verified=0, viewport_visibility_verified=0 ve display_accepted=0 değişmedi.

## Sonraki teknik eşik

- **83 ret** için yalnızca güvenli özet seviyesinde ilk başarısız kapıyı sınıflandırmak (LINE H, layer raw flags, OT51 hedef üçlüsü, terminal hizası vb.).
- Katmanın orijinal CMC renkleme alanını ayrı CRC/bit sınırı doğrulamasıyla çözmek; LINE ham CMC=256 örneklerini aynı kaynakta katman CMC'siyle birleştirmek.
- EED olmayan LINE'lar ve INSERT/blokları ayrı kabul kapısında ele almak; gerçek renk ve görünürlük için bağımsız render karşılaştırması gereklidir.

**Mimari:** LibreDWG yalnızca izole GitHub host oracle karşılaştırmasıdır; özgün C11 MusaCAD çalışma zamanı, mobil uygulama veya APK'ya bağlanmadı. Orijinal DWG'ler değiştirilmedi. PR #720 henüz main'e birleştirilmedi.
