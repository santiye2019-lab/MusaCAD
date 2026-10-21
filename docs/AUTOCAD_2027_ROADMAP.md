# MusaCAD — AutoCAD 2027 eşdeğerlik hedefi

**Kullanıcı kararı: 21 Ekim 2026.** AutoCAD 2027, MusaCAD'in ileriye dönük
**tam işlevsel eşdeğerlik karşılaştırma hedefidir**. Var olan 51 yetenek ailesinin
kimlikleri, çalışma geçmişi ve 2026 için tutulan bağımsız kanıtlar silinmez veya
örtük biçimde 2027 onayına dönüşmez.

Ana makine-okunur hedef ve sıra: [AUTOCAD_2027_TARGET.json](AUTOCAD_2027_TARGET.json).
2026 tarihsel referansları:
[AUTOCAD_2026_PARITY_MATRIX.json](AUTOCAD_2026_PARITY_MATRIX.json)
ve [AUTOCAD_2026_ACCEPTANCE_51.json](AUTOCAD_2026_ACCEPTANCE_51.json).

## Öncelik, gerçek işlevsel kanıt ve ölçüm

- **P0:** DWG/DXF gerçekten okuma, nesne/katman/ID/renk/yerleşim muhafazası,
  hatasız kaydetme/geri açma, temel 2B vektör geometri ve BLOCK/XREF.
- **P1:** Kalan bütün 2B çizim-düzenleme, yakalama, ölçülendirme,
  dinamik blok, pafta ve çıktı.
- **P2:** Yerel 3B geometri, yüzey/örgü, hassas modelleme,
  B-Rep/ACIS, Boolean/solid ve hacim hesapları.
- **P3:** Kapsamlı API/otomasyon, 2027 bulut/işbirliği, coğrafi araçlar
  ve uzmanlık araç setleri. Bağımlılıklar daha önce tamamlanabilir.

2027'nin özgül **beş ilave/yenilenmiş özellik grubu**, 51 sabit aile kimliğini
bozmadan `release_2027_deltas` içinde takip edilir: Geometry Cleanup,
Checkout, Connected References, Assistant 2027, Forma Data Management.
Bunlar ayrı "tamamlandı" sayılmaz; bağlı ailelerin alt senaryoları olarak
bağımsız kanıt gerektirir. 2027.1 iyileştirmeleri gerektiğinde alt
senaryolarda takip edilir.

**Sertifikalı 2027 eşdeğerlik: 0/51.** Bu sayı kod yokluğu değil;
aynı kaynak üzerinde referans AutoCAD 2027 testi, MusaCAD semantik
karşılaştırması, bağımsız DWG roundtrip, CI, üç platform ve bağımsız
insan incelemesi ile alınan kabulü gösterir. Tamamlanmamış alt özellik
bir aileye %100 verilmesini sağlamaz. 25 çekirdek kapısı ve AC1018
16 aşamalı ilerleme göstergeleri farklı kapsamda, ayrı kalır.

## Dosya biçimi ve tek çekirdek

AutoCAD 2027 hâlâ **AutoCAD 2018 DWG biçimi** ailesini kullanır:
harici DWG `AC1032` tanınmalı; özgün MusaCAD `MC1032` imzası
yalnızca `.mcad` biçimine aittir. Bir imza diğerinin yerine yazılamaz.
Android, iOS ve Windows aynı özgün taşınabilir C/C++ CAD çekirdeğini
kullanır; üçüncü taraf DWG SDK/LibreDWG çalıştırılmaz. Desteklenmeyen
veri saklanmadan yok edilemez; işlem güvenli biçimde reddedilir.

**Dağıtım:** Sahip açıkça son onayı vermeden APK, uygulama paketi,
üretim dağıtımı veya Cloudflare değişikliği yapılmaz.

## Kaynaklar

- [Autodesk — 2027 yenilikleri](https://www.autodesk.com/solutions/aec/autocad-updates)
- [Autodesk — 2027 sürüm notları](https://help.autodesk.com/view/BLDSYS/2027/ENU/?contextId=WHATS_NEW_2027_ACD)
- [Autodesk — DWG dosya biçimi uyumluluğu](https://help.autodesk.com/view/ACADWEB/ENU/?caas=caas%2Fsfdcarticles%2Fsfdcarticles%2FAutoCAD-drawing-file-format.html)
