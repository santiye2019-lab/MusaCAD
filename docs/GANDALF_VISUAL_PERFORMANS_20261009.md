# MusaCAD — görsel mühendislik denetimi, 2025 poz kitabı ve büyük DWG performansı

## Yeni kullanıcı arayüzü
Gandalf AI panelinin üstünde iki **ayrı** kontrol:
1. **PDF görüntüle** — son yanıtın PDF belgesini açar.
2. **Poz kitabı yükle** — kullanıcıya ait 2025 ÇŞİDB *İnşaat ve Tesisat Birim Fiyatları* PDF dosyasını Android dosya seçicisinden **cihaz içine** alır ve çevrim dışı indeksler. Kitabın tamamı GitHub/APK içine yayımlanmaz. Cihazda önceden kurulu ise durumu söyler.

Gandalf mesaj listesinin sağ tarafında görünen, solmayan kaydırma çubuğu bulunur. 
Üstteki küçük LED:
- **Denetleniyor**: /health yanıtı bekleniyor;
- **Çevrimiçi**: gerçek HTTPS MusaCAD AI gateway /health yanıtı onaylandı; bu tek başına model yanıtı değildir;
- **AI bağlı**: gerçek AI/model yanıtı başarılı;
- **Erişim yok / AI erişilemedi**: ağ veya AI çağrısı başarısız;
- **AI ayarsız**: derlemede gerek duyulan HTTPS sunucu bağlantıları yok.

Gösterge 20 saniyede bir yalnızca panel açıkken yenilenir; sağlık sorgusu **proje, CAD, sohbet, kimlik veya poz kitabı içeriği** göndermez. Çizim görüntülerinin sunucuya iletilmesi için önceki ayrı kullanıcı izni korunur.

## Görsel proje analizi
- İzinli çevrim içi AI, gerçek çizimden üretilen 3x3 bölge görsellerini ve mümkün olduğunda kat/kesit/vaziyet başlıklarının ayrı yakın çekim adaylarını inceler.
- Çıktı artık **görsel mühendislik proje denetim raporu** başlığıyla ve bölüm bazında düzenlenir: inceleme kapsamı, görüntü bölgesi gözlemleri, görünüm başlığına yakın detaylar, bağımsız vektör/ölçü kanıtları, görsel kapsam, teknik ön metraj ve 2025 poz adayları.
- Her AI bulgusu için bölge/görünüm, gerçekten okunabilen teknik veri, doğrulama derecesi, muhtemel etkiler ve kontrol önerisi istenir.
- Görsel bölge taraması bir projenin **tüm** DWG layout/kat paftalarının incelendiği anlamına gelmez. Model alanı okunamayan/kapsam dışı pafta için uygunluk iddia edemez. Yakın-plan başlık çevresi tam pafta sınırı değildir.
- 2025 poz eşleştirme yerelde aday seviyesindedir. Malzeme, çap/DN, PN, standart, birim ve montaj kapsamı onaylanmadan kesin keşif oluşturulmaz. 2025 fiyatı güncel fiyat sayılmaz.

## Büyük DWG açılışı ve yakınlaştırma
- Native DWG'nin hızlı 2B sahnesi mümkünse **önce** görüntülenir. Tam, düzenlenebilir DXF vektör modeli yükleme iş parçacığında hazırlanırken kullanıcı ilk sahneyi görebilir.
- Bu ilk sahnenin renkleri/özellikleri **geçici önizleme** olabilir; vektör modele geçince yetkili çizim rengi kullanılır. Bu sınır kullanıcının önünde açıkça belirtilir. Native sahne verisi başarısızsa tam vektör açılışına devam edilir.
- Büyük DXF ve DWG dosyalarının her çizgisini içeren 1200px raster önizleme zorunluluğu azaltıldı.
- Yakınlaştırma/kaydırma sürerken 18.000+ görünür vektörlü projelerde bir ekran pikselinden küçük detaylar geçici sadeleştirilir. Uzun çizgiler/boru güzergâhları korunur. Parmak hareketi durunca **tam vektör** çizilir ve hiçbir CAD nesnesi belge içinden silinmez.
- Bu değişiklik büyük dosyaların açılışını ve hareket sırasındaki çizim yükünü azaltmayı hedefler. *Her cihazda, her DWG büyüklüğünde sabit 60 FPS garanti edilmez.* Gerçek telefon ve gerçek ağır DWG örnekleriyle ölçüm gerekir.

## Teknik doğrulama
- `CadNavigationPolicyTest`: yoğunluk, minik nesne LOD ve uzun güzergâhların korunması.
- `MusaCadVisualPerformanceContractTest`: görsel AI akışı, 2025 kitabı yükleme, canlı bağlantı LED'i, kaydırma çubuğu, hızlı DWG açılışı, tarihlendirilmiş poz adayları.
- `MusaAiPanelContractTest` ve `UiResponsivenessContractTest`: mobil arayüz/işlem değişiklikleri.
- CI Android lint/derleme, bağımsız AI worker testleri ve uygulama gerçek cihaz performans testi.

**Saha testi:** Silivrikapı Spor Köyü Kafe Mutfak ve daha büyük çok katlı DWG'lerde açılış süresi, ilk sahneye kadar bekleme, yakınlaştırma FPS/jank, pafta başına AI görsel kapsaması ve poz adayları tek tek doğrulanmalı.
