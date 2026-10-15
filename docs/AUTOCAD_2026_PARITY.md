# MusaCAD ↔ AutoCAD 2026: gerçek işlevsel eşdeğerlik

## Birincil ürün hedefi

**Hedef, AutoCAD 2026 masaüstü sürümünün tüm işlevsel özellikleriyle
eşdeğer çalışma, aynı davranış ve veri güvenilirliğidir.** Özellikle çizim,
nesne oluşturma/düzenleme, 2B/3B modelleme, görünüm, koordinatlar, stiller,
bloklar, harici referanslar, sayfa/çıktı, API/otomasyon, iş birliği ve ürün
paketine dâhil yedi sektör araç seti kapsam dışı bırakılamaz.

MusaCAD'in kullanıcıyla ayrıca tasarladığı mühendislik yapay zekâsı, poz/metraj
analizleri, disiplin kontrolü ve benzeri fark yaratan modüller **ikinci
aşamadadır**. Bunlar AutoCAD 2026 işlevsel eşdeğerliğini yükseltmek için
puan olarak kullanılamaz. Marka, telifli kod veya kullanıcı arayüzü görsellerini
kopyalamak değil, bağımsız ve işlevsel olarak eşdeğer ürün amaçlanır.

## İki yüzde birbirine karıştırılamaz

1. Önceki `docs/CAD_2026_PROGRESS.json` içindeki **%61**, MusaCAD'e özgü
   daraltılmış *geliştirme planı puanının* ilerlemesidir. AutoCAD 2026
   eşdeğerliği değildir. Bu hedef için ana ilerleme göstergesi olamaz.
2. AutoCAD 2026 işlevsel eşdeğerlik yüzdesi, **henüz ölçülebilir değil**.
   `AUTOCAD_2026_PARITY_MATRIX.json` başlangıç düzeyinde **51 yetenek
   ailesi** içerir; tam komut/seçenek/senaryo envanteri değildir. Şu an
   23'ü kısmi kod, 7'si belgelenmiş engel, 21'i inceleme bekliyor.
   Hiçbirine bağımsız referans eşdeğerlik onayı verilmedi. Bu, yazılımın
   %0 çalıştığı anlamına GELMEZ. Ayrıca 23/51 oranını eşdeğerlik
   yüzdesi diye sunmak da yanlıştır.

## AutoCAD 2026 resmî komut dizini – birinci karşılaştırma dilimi

`docs/AUTOCAD_2026_COMMAND_CATALOG.json` dosyası, Autodesk'in
**AutoCAD 2026 resmî alfabetik komut dizininden** alınmış **247 ayrı
komut/komut satırı varyantını** içerir. Kapsam şu an: `+`/`-` komut
satırı varyantları, `3D*`, A, B ve C harfinin başlangıcı. Bu,
AutoCAD 2026'nın tüm komutları değildir; C–Z'nin kalan bölümleri,
sistem değişkenleri, alt-komut seçenekleri, UI davranışları ve sektör
araç setlerinin ayrı kataloglarının eklenmesi zorunludur.

`tests/autocad_2026_parity_gate.py` hiçbir komutun yalnız ismen
bulunması veya kodda bir eyleme eşlenmesiyle eşdeğer sayılmasına izin
vermez. Komut bazında bağımsız **gerçek AutoCAD 2026 davranışı, gerçek
DWG kaydet–yeniden açma ve Android telefon testi** gerekir.
`tests/AutoCAD2026CommandRouterSmokeTest.java` içindeki 25 adın
**11 tanesi yalnızca komut yönlendiricide tanınmaktadır**,
**14 tanesi mevcut yönlendiricide henüz eyleme eşlenmiyor**.
Bu testler **işlevsel eşdeğerlik testi değildir**, hiçbir komuta
`verified` durumu vermez.

Autodesk'in 2026 komut listesi: 
https://help.autodesk.com/view/ACD/2026/ENU/?page=commands&q=*

## Ölçüm kuralı

1. Autodesk'in **AutoCAD 2026 sürümüne ait resmî** komut, alt-komut,
   seçenek, dosya nesnesi, değişken, API ve ürünle verilen araç seti
   katalogları tamamlanıp sabit sürüm envanterine dönüştürülür.
2. Bir eşdeğerlik testi, tek tek kullanıcı davranışı ve beklenen çıktı
   olarak tanımlanır; saymayı kolaylaştırmak için komutlar birleştirilmez
   veya bölünmez. Gerekli parametre/opsiyonlar ayrı kontrol edilir.
3. Aynı girdi, gerçek AutoCAD 2026'da ve MusaCAD'de işlenir. Bağımsız
   referans, doğruluğu bilinen DWG/DXF nesne/katman/layout/öznitelik
   verileri, düzenleme sonrası yeniden açma, 3B B-Rep topoloji ve çıktı
   karşılaştırması ile kanıtlanır. Ekran görüntüsünün benzemesi yetmez.
4. Android telefon kabulü ve yeterince büyük gerçek saha çizimleri
   gerekir. Desteklenmeyen, yalnız görüntülenebilen, yaklaşık
   modellenen veya sadece sentetik test geçen bir özellik **eşdeğer
   olarak sayılmaz**.
5. Ancak **tam katalog kilitlenip** kabul senaryoları oluşturulduğunda
   `100 × tüm referans davranışlarıyla eşdeğerliği kanıtlanan senaryo
   sayısı / toplam 2026 referans senaryo sayısı` hesaplanır.
   Tam eşdeğerlik **%100** yalnız tüm zorunlu senaryolar geçtiğinde.
   Komut adı tanınması, testin derlenmesi, bir API sınıfının bulunması
   veya sadece çizim gösterilmesi yeterli değildir.
6. Katalog kapsamı genişlerse payda sürüm kontrollü şekilde güncellenir.
   İlerleme yüzdesi düşebilir; hiçbir eksik özellik uydurma puanla
   kapatılmaz.

## Bugün doğrulanabilen başlıca sınırlar

- DXF okuma, DXF kaydetme, vektör görüntüleme, sınırlı düzenleme, ölçme,
  PDF baskı ve bazı nesne ayrıştırma araçları mevcut. Bunların hiçbiri
  bütün AutoCAD 2026 seçenekleriyle birebir doğrulanmış değil.
- Android uygulamasında modern **AC1032 DWG için eksiksiz, bağımsız
  doğrulanmış DWG Save As** bulunmuyor. Eski R2000/AC1015 DXF→DWG
  denemesi yalnız bilgisayar tarafında ve sınırlı.
- 3B SAT/B-Rep okuyucu ve üçgenleme, eksiksiz **EXTRUDE, REVOLVE,
  SWEEP, LOFT, UNION, SUBTRACT, INTERSECT, SOLIDEDIT** yeteneği
  anlamına gelmiyor. Tam katı modelleme çekirdeği ayrıca gerekiyor.
- Dinamik bloklar, tam XREF, kapsamlı DWG nesne koruma, AutoLISP,
  ObjectARX/.NET, Autodesk Docs, tarama/markup özellikleri ve yedi
  sektör araç setinin tam işlev eşdeğerliği kanıtlanmış değil.
- Mühendislik yapay zekâsı ayrı MusaCAD yenilik envanterinde tutulacak;
  bu eşdeğerlik paydasını şişirmek için kullanılmayacak.

## Kaynaklar

- Autodesk'in 2026 özellik ve yedi sektör araç seti tanıtımı:
  https://www.autodesk.com/tr/products/autocad/features
- Autodesk resmî AutoCAD 2026 alfabetik komut dizini:
  https://help.autodesk.com/view/ACD/2026/ENU/?page=commands&q=*
- Autodesk 2026 çoklu çizgi semantiği (PEDIT ve legacy POLYLINE ayrımı):
  https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-Core/files/GUID-392BF13C-D9E7-47A8-8E07-435296332279.htm
- Autodesk 2026 çoklu ölçüm seçenekleri (MEASUREGEOM):
  https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-Core/files/GUID-5D5B0EE1-DD90-47AE-8A55-642FBFF5E4E4.htm

**Güvenlik:** Son kabulden önce `main` birleştirmesi, üretim APK
dağıtımı ve canlı servis değişikliği yapılmaz. Yeni envanter yalnız
geliştirme dalında saklanır.
