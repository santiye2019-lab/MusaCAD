# MusaCAD CAD motoru stratejisi ve genişletme kararı — 15.10.2026

**Durum:** Mimari hedef ve değerlendirme kararı; **motor değiştirme kararı değildir**. `main`, mevcut APK, kullanıcı verileri ve mevcut DWG/DXF iş akışları değiştirilmedi.

## Ürün hedefi

1. **Önce AutoCAD 2026 ile tam işlevsel eşdeğerlik:** komutlar, nesne modeli, hassasiyet, çizim/düzenleme, parametrik çalışma, bloklar/XREF, gerçek 3B B-Rep, veri bütünlüğü, DWG okuma/yazma, ölçülendirme, pafta ve baskı, otomasyon, iş akışları. AutoCAD 2026 özellikleri bazında *ayrı, denetlenebilir* bir kabul matrisi tutulacak. Mobil/platform farkları için uygulanamayan özellikler ayrıca raporlanacak; sessizce "tamam" sayılmayacak.
2. **Sonra AutoCAD'in ötesi:** kullanıcı tarafından tasarlanan yeni CAD araçları ve kanıta dayalı, disipline özgü yapay zekâ mühendislik analizleri. Bunlar AutoCAD eşdeğerliği paydasına eklenmeyecek; ayrı MusaCAD yenilikleri envanterinde izlenecek.
3. Önceki **%61 yalnız iç geliştirme planının puanıdır**; AutoCAD 2026 eşdeğerlik yüzdesi değildir. Nesnel kabul matrisi kurulup sınanmadan gerçek eşdeğerlik oranı ilan edilmeyecek.

## Hazır motoru baz alıp kendi yeteneklerimizi üstüne inşa etme

Evet: CAD veri/çizim/katı geometri motorunu yeniden icat etmek yerine mevcut bir çözümün belgelenmiş ve **lisansça izin verilen** API/uzantıları üzerinde MusaCAD modülleri geliştirilebilir. Kapalı kaynak Autodesk kodunu alıp değiştirerek dağıtmak, lisans izni olmadan mümkün değildir; kodun sonradan değiştirilmesi bu hakkı vermez. SDK ile **uzatma/entegrasyon** ve gerçek motor **kaynak kodunu değiştirip yeniden dağıtma** hakları birbirinden farklıdır.

Aday yollar (hiçbiri henüz seçilmedi):

| Aday | Güçlü tarafı | Gerçek kısıt / karar kapısı |
| --- | --- | --- |
| **ODA Drawings SDK** | Android dâhil mobil DWG görüntüleme, oluşturma, düzenleme ve kaydetme SDK'sı; uygulama düzeyinde API ile genişletme | Ücretli üyelik/dağıtım koşulları; özellikle çekirdek kaynak koduna erişim yalnız ilgili üst üyelikte. Sahada Android ARM64 testleri ve lisans incelemesi gerekli. |
| **Autodesk AutoCAD OEM** | Lisanslı AutoCAD tabanlı, ObjectARX/AutoLISP/.NET ile özelleştirilebilir ürün platformu | OEM lisansı; Android'de yerel mevcut MusaCAD çekirdeğine gömülebilirlik kanıtlanmış değil. AutoCAD'in bulutla bağlantılı bazı özellikleri OEM'de yok. |
| **Autodesk RealDWG** | Autodesk'in DWG/DXF okuma-yazma SDK'sı | **Tam AutoCAD düzenleme/3B motoru değildir**; resmi RealDWG 2026 gereksinimleri Windows'ı listeliyor, doğrudan Android çekirdeği diye kabul edilemez. |
| **Open CASCADE Technology (OCCT)** | Gerçek katı modelleme için kaynak kodu erişilebilir B-Rep/geometri çekirdeği | LGPL 2.1 + istisna yükümlülükleri; tek başına AutoCAD komutlarını, UI'yı veya modern DWG dosya modelini sağlamaz. Android bütünleşmesi ve performans çalışması gerekir. |
| **Mevcut GNU LibreDWG yolu** | Depoda halihazırda var; DWG/DXF çözücü ve kısıtlı R2000 yazıcı denemesi | GPLv3+ yükümlülükleri ve modern DWG yazma/karmaşık nesne eşdeğerliği eksikleri; ürünün tamamında lisans uyumluluğu ayrıca incelenmeli. |

### Tercih edilen mühendislik yaklaşımı (deneme kararı)

Mevcut MusaCAD kodlarını **silmeden**, motor bağımsız bir uygulama cephesi oluştur:
- `DrawingBackend`: open, queryEntities, transactions, saveAs, reopen, stable IDs, undo/redo, diagnostics.
- `GeometryBackend`: tam 2B eğriler, tolerans/koordinat sistemi, B-Rep ve boole işlemleri.
- `RenderBackend`: 2B/3B görünüm, görünüm/katman tutarlılığı, büyük çizim performansı.
- `EngineeringExtension`: yerel nesne kimliği + geometri + metin + görsel kanıta dayalı AI/mekanik/statik/elektrik inceleme; tespit ve önerilen düzeltme kullanıcı onayına tabi.
- `CustomFeatureRegistry`: ileride kullanıcının tarif ettiği AutoCAD dışı araçları mevcut dosyaları bozmadan eklemek için komut, işlem, model ve test bağlama noktaları.

**Uygulama öncesi zorunlu kanıt:** Lisans/hukuk onayı; ARM64 Android örnek derlemesi; özgün DWG ile aynı dosyayı aç-düzenle-kaydet-yeniden aç, bağımsız CAD okuyucuyla koordinat, eğri, yazı, blok, XREF, düzen, ölçü, 3D ve metadata doğrulaması; başarısız/eksik öğeyi gizlemeyen rapor; büyük paftalarda performans; geri alma ve kaynak dosya koruması. Bu kapıları geçmeyen motor seçilmez.

**Şimdiki durum:** ODA, OEM, RealDWG veya OCCT için lisans satın alınmadı, yazılım indirilip entegre edilmedi, motor değiştirilmedi. Mevcut çalışma dalındaki testler AutoCAD 2026 eşdeğerliği kanıtı değildir.

## Resmî belgeler (15.10.2026 tarihinde incelendi)

- ODA Drawings (Android ve DWG düzenleme): https://www.opendesign.com/products/drawings
- ODA üyelik, kaynak kodu erişimi ve dağıtım: https://www.opendesign.com/oda-membership
- Autodesk OEM programı: https://www.autodesk.com/partner-program/oem-software
- AutoCAD OEM arayüzleri ve sınırlamalar: https://forge.autodesk.com/developer/overview/autocad-oem
- RealDWG SDK ve 2026 sistem gereksinimleri: https://forge.autodesk.com/developer/overview/realdwg-api
- Open CASCADE Technology lisansı: https://github.com/Open-Cascade-SAS/OCCT/blob/master/dox/license.md
- GNU LibreDWG lisansı: https://www.gnu.org/software/libredwg/
