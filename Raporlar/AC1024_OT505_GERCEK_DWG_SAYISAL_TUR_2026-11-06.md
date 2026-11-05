# MusaCAD — Karagümrük gerçek DWG dosyalarında OT505 sayısal sınıf türü

**Tarih:** 2026-11-06
**PR:** https://github.com/santiye2019-lab/MusaCAD/pull/720
**Gerçek kaynak test commit'i:** b0d8c96082f31ccdf2dd42a0f428998bb86f7827
**Başarılı gerçek DWG matrisi:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42110575230
**Kaynak kanıtı:** Bu çalışmanın musacad-real-native-stages.json tanı artifact'ı.

## Doğrulanmış dar sonuç

- Gerçek DWG matrisinde **23 dosya** incelendi; **10 dosya AC1024** sürümündeydi.
- Karagümrük'ün **9 AC1024 dosyasında** özgün fiziksel AcDb:Classes bölümü doğrulanmış, mantıksal bölümü çıkarılmış, R2010 bit-count öneki ve **sayısal sınıf kayıtları** ayrıştırılmıştır.
- Bu 9 dosyada toplam **532 sayısal class kaydı** çözümlenmiştir (sırasıyla 59, 67, 56, 56, 56, 70, 53, 53, 62).
- **9/9** dosyada çizim-yerel classnum **505** bulunmuştur; her birinde item_class_id=0x1F3 (**OBJECT**) doğrulanmıştır. ENTITY=0x1F2 sonucu yoktur.
- **Otomasyon tesisi çizimi** için eşdeğer sayısal sınıf kabulü oluşmamıştır. Bu dosya 9/9 başarı paydasına dahil edilmemiştir.
- Kaynak çizimlerin yeniden yazıldığına ilişkin bir kabul yoktur; stage source_unchanged bayrağını doğrular.

## Sınırlamalar

1. **505 bir çizim-yerel dinamik sınıf numarasıdır.** Dokuz dosyadaki aynı sayı veya aynı item_class_id, bunların aynı sınıf adına ve semantik role sahip olduğunu kanıtlamaz.
2. Henüz **özgün Unicode TU appname/C++ classname/DXF classname** alanları çözülmemiştir. OT505 = MATERIAL, XREF, LTYPE, PLOTSTYLE veya belirli bir malzeme/tabaka sonucu **çıkarılamaz**.
3. Bu test **layer görünürlüğü, BYLAYER renk, blok gösterimi, koordinat, çizim eşdeğerliği, DWG yazımı veya APK** kabulü değildir.
4. Yeni tests/real_dwg_classes_corpus_evidence.py yalnızca kaynak-kökeni doğrulanmış sayısal sınıf metadata'sının toplu özetini çıkarır. Adları, çizim baytlarını, koordinatları veya handle listesini yayımlamaz; kaynak-dışı veya çelişkili delil için başarısız olur. **Bu yeni kontrolün kendi CI kararı ayrıca takip edilmelidir.**

## Sonraki teknik kapı

Orijinal AC1024 Classes bit-count sonundaki string-present işaretinden başlayarak **sınır kontrollü UTF-16/TU string-stream konumunu** bulmak; sonra her çizim-yerel 505 kaydının üç string'ini ayrı ayrı doğrulamak. Önceki sayısal kanıt, bu adları uydurma veya çizime sessizce uygulama yetkisi vermez.
