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

## Büyük LibreDWG JSON çıktıları (PR #720 sınır düzeltmesi)

İlk testte LibreDWG JSON çıktısı 128 MiB toplam dosya sınırına takılarak
`libredwg_json_exceeds_bounded_limit` durumuna düşüyordu. Host-only
karşılaştırıcı artık tüm JSON'u belleğe yüklemez: sabit sürümlü `ijson`
olay ayrıştırıcısı ile `OBJECTS` içindeki tür alanlarını tek geçişte
sayar. Ham koordinat, metin, handle ve layer adları rapora aktarılmaz.
Toplam JSON boyutu 128 MiB'yi aşsa da sayım devam eder; korunan sınır
tek bir çizimdeki 2.000.000 nesne kayıt sınırıdır. CI testi özellikle
128 MiB üzeri JSON ve iç içe sahte kayıtları doğrular. Bu değişiklik
LibreDWG'yi sadece izole host testine ekler; MusaCAD çekirdeğine eklemez.

## LibreDWG'nin geçersiz JSON çıktısına karşı doğrudan tür sayımı

Karagümrük AC1024 dosyasında normal JSON (150,8 MB) ve minJSON
(101,5 MB) geçersiz belirteç içerdiğinden, yalnızca JSON ayrıştırıcısı
ile kesin referans envanteri çıkartılamıyordu. GitHub Actions artık
bunları sırayla dener ve ikisi de geçersizse `tests/libredwg_host_type_inventory.c`
adlı **yalnızca CI host'unda derlenen** yardımcı programı kullanır.
Bu program pinned LibreDWG ile aynı DWG'yi okur, yalnızca çözümlenmiş
`Dwg_Object` kayıtlarının türlerini yayımlar; koordinat/handle/katman
adı/metin içeriğini asla çıktıya almaz. Ham çizim veya ham LibreDWG
JSON verisi artifact olarak yüklenmez. Çözücü uyarı bitleri
`reference_decoder_warning_mask` alanında saklanır.
Her üç yöntem başarısızsa iş akışı kırmızı kalır; başarısız bir
referans okuması başarı diye raporlanmaz.

Bu doğrudan host yardımcı aracı GPL LibreDWG kütüphanesine **sadece
geçici GitHub Actions bilgisayarında** bağlanır. MusaCAD'in özgün
çekirdeğine, Android APK'sına veya dağıtım çıktısına eklenmez.

## AC1024 Karagümrük: ilk özgün okuyucu karşılaştırması

GitHub'daki Karagümrük pis su çizimi **AC1024** biçimindedir ve mevcut
MusaCAD native sürüm sondasının gerçek AC1024 parser kabulü yoktur.
Bu nedenle test-only `real_dwg_native_stage_probe.c`, disk üzerindeki
kaynak dosyayı değiştirmeden, geçici RAM kopyasının ilk altı baytını
yalnızca tanı amacıyla `AC1018` biçimine alıp mevcut doğrulayıcılar ile
korumalı bölüm haritası, sayfa haritası, bölüm sayfaları ve CRC tabanlı
handle indeksini sınar. Çıkmadan önce bellek kopyasının imzası geri
yüklenir. Bu giriş `ac1024_family_hypothesis_only` aşamasıdır; gerçek
AC1024 biçimi olarak **asla accepted raporlanmaz**.

Karagümrük pis su dosyasında bu test **145.279 aday indeks** buldu;
ayrı LibreDWG host sayacı da **145.279 referans nesne kaydı** bildirdi.
Sayı eşleşmesi tesadüfi veya kısmi indeks / sınıflandırma hatalarını
dışlamaz. 10 AC1024 çizimden 9'u aday indeks aşamasına ulaştı;
otomasyon çizimi korumalı dosya başlığından sonra fiziksel sayfa
haritasında durdu. Bunlar henüz AC1024 tam okuyucu uyumluluğu değildir.

İzleyen *ayrı* testte aynı geçici RAM kopyasıyla, geçerli aday indeksi
olan ve kaynak boyutu en çok 12 MiB olan çizimlerde mevcut R2004
kayıt-çerçeve CRC okuyucusu bütün indeksleri sınar. Sadece ham numerik
BS türleri 17, 18, 19, 77 için aday sayımlar çıkarılır; bunlar tek
başına sınıf/nesne adı, gövde koordinatı, embedded self handle,
owner/layer bağlantısı veya AutoCAD eşdeğerliği kanıtı değildir.
Eşleşen sayılar bile raporda yalnızca **candidate count correlation**
olarak kalır; `entity_parity=NOT_MEASURABLE` değişmez.
Yetersiz veya yanlış indeks/CRC durumunda kesin eşdeğerlik iddiası
üretilmez. Çizim kaynağı SHA-256 değeri iki kez denetlenir.

## Kayıt ofseti karşılaştırması (R2010)

İlk kapsamlı orijinal Karagümrük pis su taramasında R2004 kayıt
çerçevesi yalnızca **558 / 145.279** aday kaydın CRC'sini doğruladı;
**144.721** kayıt için kabul üretilmedi. ODA'nın yayımlanmış
DWG format tanımı R2010'un R2004 bölüm yapısı ile R2007
ayrılmış string/handle alanını ve farklı object-type kodlamasını
birleştirdiğini belirtir:
https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf

Bu nedenle sonraki CI tanısı, doğrulanmış `AcDbObjects` ve
`AcDb:Handles` ham bölüm baytlarında *sürekli (continuous) nesne
ofsetleri* hipotezini eski sayfa-içi ofset modelinden ayrı olarak
test eder. İkinci harita da kendi CRC16 doğrulamasından geçer;
kayıt CRC'leri her orijinal aday ofseti için ayrı doğrulanır.
Sürekli ofsetli indeks profili Karagümrük'teki 10 AC1024 çizimden
hiçbirinde mevcut katı ve CRC denetimli indeks kabulünü geçemedi.
Bir diğer olası alternatif olan `page_absolute_handles` profili
aynı kaynak üzerinde ayrı sınanır. Her profil özgün handle-map CRC'si
ve orijinal kayıt çerçevesi CRC ölçümlerini ayrı raporlar;
hiçbiri son kullanıcı çizim modeline otomatik kabul edilmez.

Bu yöntem yalnızca geçici, izole tanı kodudur. Tam AC1024
nesne sınıfı/handle self/owner/layer/renk doğrulaması ya da
normal uygulama açılışı değildir. Yanlış ofset profilini
başarıya yükseltmek yasaktır.

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
