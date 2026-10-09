# Gandalf AI — bütün paftalarda kaynaklı mühendislik inceleme hedefi

## Hedef
MusaCAD açık proje paketindeki DWG/DXF dosyalarını, model/layout görünüm ve
pafta başlık adaylarını birlikte envantere alır. Görsel JPEG parçaları,
DWG kaynak kimlikleri, dünya koordinatı, katman/metin/ölçü ve varsa
resmî poz analizi ayrı doğrulanmış kanıtlar olarak tutulur. Nihai PDF
**yapılmamış analizleri yapılmış gibi göstermez**.

## Uygulama aşamaları ve kabul koşulları
1. **Kanıt kapsam cetveli — PR #212.** Her tespit edilmiş kat/kesit/vaziyet/
   çatı başlığına kaynak kimliği ve koordinat yaz; görünümün çerçevesi
   bulunmadıkça sadece 'başlık adayı' statüsü göster. 429 / timeout halinde
   tamamlanan bölge sayısı açıkça yazılır.
2. **Çizim/paket envanteri.** Kullanıcının açtığı bütün ilgili DWG dosyaları,
   Model ve layout'lar; referans dosya (XREF) ve görünmez katman eksikleriyle
   raporlanır. Şu an tek aktif çizimdeki 3x3 tarama, tüm proje kapsamı değildir.
3. **Pafta sınırı ayrıştırma.** Pafta çerçevesi, viewport, antet ve kullanıcı
   seçili sınırları ilişkilendirilir; yalnızca başlık çevresinden tam pafta
   çıkarımı yapılmaz. Eksik pafta için kullanıcıya doğrulama isteği verilir.
4. **Aşamalı görsel ve vektör inceleme.** Her doğrulanmış pafta önce düşük
   çözünürlüklü genel görünüm, sonra gerekli ayrıntı ve sayısal kaynak
   eşlemesiyle incelenir. Her bulgu pafta ID, bölge, koordinat, DWG sourceId,
   görülen değer ve güven düzeyini taşır.
5. **Disiplinler arası ve paftalar arası kontrol.** Kolon devamlılığı, çap,
   malzeme, kot, debi/hava yönü, cihaz yerleşimi, kesit ve vaziyet ilişkileri
   karşılaştırılır. Yalnızca aynı referans sistemine bağlanabilen ölçülerle
   kesin uyuşmazlık önerilir.
6. **Kota / kesinti yönetimi.** Gemini HTTP 429'da AI taraması otomatik
   tekrarlanmaz, görev durur. Tamamlanan bölgelerin başarı kayıtları
   cihazın özel saklama alanında kullanıcı onaylı güvenli oturum süresiyle
   tutulur; kullanıcı devam dediğinde yalnız eksik paftalar denenir.
   Farklı projenin, layout'un veya kaynak revizyonunun sonuçları birbirine
   karıştırılmaz. Yalnız yerel CAD bulguları kota olmadan raporlanır.
7. **Keşif.** Doğrulanabilir metrajları birim/ölçek ve kaynaklarla eşleştir.
   2025 kitabı 2026 güncel fiyat diye kullanılamaz. Montaj kapsamı, çap
   sınıfı, malzeme ve ölçü birimi doğrulanmadan ÇŞİDB pozu/fiyatı kesin
   tahakkuk kalemi olamaz.
8. **Rapor.** Yönetici özeti; pafta listesi ve kapsam yüzdesi; doğrulanan
   kusurlar/öneriler/belirsizlikler; kat-kesit-vaziyet koordinasyonu; kaynaklı
   metraj ve keşif adayları; resimli pafta ekleri; incelenemeyen yerler.
   Kullanıcı izni olmadan DWG üzerinde düzeltme uygulanmaz.

## Sınırlar
Güncel kodun 3x3, en fazla 12 başlık çevresi taraması *tüm paftalar*
doğrulaması değildir. #212 yalnız kanıt/kapsam raporlamasını iyileştirir.
Gerçek tam pafta analizi için özellikle 2–6. aşamaların ayrı uygulaması
ve gerçek telefon üzerinde DWG saha doğrulaması gereklidir.
