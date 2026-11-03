# GitHub'da izole LibreDWG referans testi — Karagümrük pilotu

Bu test **MusaCAD uygulamasına LibreDWG eklemez**. GitHub Actions geçici
Ubuntu 24.04 bilgisayarında LibreDWG'nin sabit
34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43 commit'inden
dwgread derler. MusaCAD'in mevcut özgün core/native C kütüphanesi ve
tests/real_dwg_native_stage_probe.c ayrıca derlenir. İkisi aynı
gerçek DWG'yi salt okunur inceler.

İlk test dosyası: **Projeler/01-KARAGÜMRÜK STADYUM - PİS SU TESİSATI.dwg**.
Manuel workflow_dispatch ve scope=all seçeneğiyle bütün public DWG
korpusu sınanabilir. Telefona veya bilgisayara program kurulmaz.

## Şu anda gerçek ölçüm

- Referans dwgread -O JSON çıktısında OBJECTS altındaki açıkça tanımlanmış
  LINE, INSERT, TEXT, LAYER vb. nesnelerin türlerine göre sayımı yapılır.
  Bilinmeyen kayıtlar sayısal type alanından uydurulmaz.
- MusaCAD'in mevcut birinci taraf C sondası aynı dosyada ulaştığı
  bölüm/sayfa/handle okuma aşamasını bildirir; bu aşama nesne sayısı değildir.
- Rapor sadece nesne tür/sayıları, native stage, kaynak SHA-256 ve
  public dosya adını içerir. Ham DWG, layer adları, koordinatlar,
  text içerikleri, handle listesi ve LibreDWG JSON dosyası yüklenmez.
- Git LFS kaynağının SHA-256 değeri testten önce ve sonra kontrol edilir;
  kaynak değişimi veya eksik indirme altyapı hatası olarak işaretlenir.
  Rapor GitHub Actions artifact olarak 14 gün saklanır.

## Kesinlikle kanıtlanmayan şeyler

MusaCAD'in mevcut native stage sondası **tam entity envanteri üretmiyor**.
Bu sebeple entity_parity = NOT_MEASURABLE, render_parity = NOT_TESTED ve
autocad_equivalence = NOT_TESTED olarak kalır. Native accepted sonucu
tam projeyi açtık anlamına gelmez. LibreDWG de nihai AutoCAD referansı
değildir.

## Sonraki gerçek geliştirme eşiği

1. Özgün MusaCAD host okuyucudan orijinal DWG entity type, source handle,
   owner, layer ve color envanterini eksiksiz dışarı verecek somut kanıt.
2. Bu envanteri aynı dosyanın izole LibreDWG referans çıktısıyla ve
   bağımsız AutoCAD çıktılarıyla karşılaştırmak.
3. Kanıtlanan eksik reader/decoder adımlarını mevcut koda düzeltme olarak
   uygulamak; yeni paralel owner/layer/block çözücüsü açmamak.

LibreDWG GPL kapsamı host test süreciyle sınırlıdır; kaynak veya binary
bağımsız MusaCAD çekirdeğine link/kopya edilmez. Önceki repodaki lisans
izleri ticari dağıtım öncesi ayrıca incelenmelidir.

İş akışı: .github/workflows/real-dwg-libredwg-host-oracle.yml
