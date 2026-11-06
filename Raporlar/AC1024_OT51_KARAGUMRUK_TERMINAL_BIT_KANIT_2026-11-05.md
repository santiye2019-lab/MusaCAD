# MusaCAD MC1024 – Karagümrük OT51 ham H-kuyruk bit kanıtı

**Tarih:** 2026-11-05  
**Durum:** Salt-okunur kaynak incelemesi; geometri/renk/görünürlük kabulü **YOK**.  
**Kaynak commit:** `a4b60eece87cdca7f7944e94fc1ea136bd1cc9df`  
**CI:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42025197268  
**Ham sayım artefaktı:** https://github.com/santiye2019-lab/MusaCAD/actions/runs/42025197268/artifacts/13413113232

## Kapsam ve sonuç

Repo `Projeler` altındaki **23 kaynak DWG** üzerinde çalışan sürüm-test matrisinin AC1024 altkümesinde, gerçek Karagümrük dosyalarından alınan **14 adet OT51 LAYER kayıt örneği** vardır. Sayımlar tamamı üzerinden veya çizim başarımı üzerinden genellenemez.

- Orijinal CRC/self-H kanıtına bağlı OT51 -> OT50 (H4) / OT42 (H3) grafiği: **14/14**.
- Sonraki ilk ham H başlığı: **14/14**, kod=5 ve boyut=0. **Bu kodun CAD semantiği henüz teyit edilmedi.**
- Kuyruğun **tamamını** sıfır artık bit ile H-belirteçlerine ayrıştırma: **0/14**.
- İlk reddetme aşaması: **SHORT_HEADER, 14/14**. Bu, dosya hasarı değil, mevcut katı H-belirteci modelinin son 1–7 biti kabul etmemesidir.
- SHORT_HEADER ardındaki kaynakla bağlı küçük son parça: **14/14**. Önceki dört tam H-belirteci sayılmıştır (**56 toplam; 14 kayıtta 4’er tane**). Bunlardan **14** adedi H5/0 bayt biçimindedir.
- Parça uzunlukları: **2 bit = 10**, **4 bit = 4**, diğer 1–7 bit uzunlukları = 0.
- Parça bit içerikleri: **12 tamamen sıfır**, **2 sıfır olmayan**. *Sıfır olması, dolgu olduğu anlamına gelmez.*

### Disiplinlere göre kaynak-bazlı sayım

| Karagümrük DWG | Örnek kayıt | 2 bit | 4 bit | Sıfır | Sıfır olmayan |
|---|---:|---:|---:|---:|---:|
| Pis su tesisatı | 2 | 1 | 1 | 2 | 0 |
| Yağmur tesisatı | 4 | 4 | 0 | 4 | 0 |
| Temiz su tesisatı | 4 | 1 | 3 | 4 | 0 |
| Yangın tesisatı | 2 | 2 | 0 | 2 | 0 |
| Isıtma ve soğutma tesisatı | 2 | 2 | 0 | 0 | 2 |
| **Toplam** | **14** | **10** | **4** | **12** | **2** |

## Güvenlik / değerlendirme kararı

1. **H5/0** yalnızca ham bit-şekli kanıtıdır. Renk, çizgi tipi, materyal, plotstyle, XREF, blok ya da görünürlük rolü çıkarılamaz.
2. Kalan bitlere *dolgu* veya *geçerli H başlığı* denilmez; 2 sıfır-olmayan örnek bunu özellikle engeller.
3. Tam-kuyruk ayrıştırıcısı **fail-closed** kalır; `display_accepted=0`, kaynak DWG üzerinde yazma **yoktur**.
4. Yeni `prefix_h_code/size[4]` kayıtları yalnızca CRC-doğrulanmış dört tam H başlığının biçimlerini güvenli şekilde saymak içindir. Hedef nesne çözümü ve semantik henüz yoktur.
5. Kabul sırası: gerçek kaynak kanıtı -> sentetik negatif testler -> Linux/Windows/macOS/iOS ABI -> Android kalite -> yalnızca kanıtla desteklenen görüntüleme.

**Sonraki araştırma:** Dört tam H başlığının pozisyona göre kod/uzunluk dağılımını karşılaştır; son 2/4 bitin kaynak içindeki yapısal rolünü teyit etmeden katman özelliklerini bağlama.
