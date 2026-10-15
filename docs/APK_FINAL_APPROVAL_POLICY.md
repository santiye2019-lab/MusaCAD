# MusaCAD — yalnızca son açık onayla APK üretimi

**Karar (15.10.2026):** Geliştirmeler devam ederken **hiçbir APK
üretmeyeceğiz**. Kullanıcının tüm geliştirmelerden ve kabul kontrollerinden
sonra vereceği **yeni ve açık son onay** olmadan normal, debug, pilot,
imzalı veya üretim APK'sı oluşturulmayacak. Önceki test APK'sı izinleri
yeni APK'ya onay sayılmayacak.

## Geliştirme sırasında yapılabilecekler

- Ayrı geliştirme dalında kaynak kod değişikliği, GitHub PR, birim
  testleri, host native converter testi, Android lint ve Java derleme
  kontrolü.
- Testler geçtikçe onaylı kaynak kodu `main`e birleştirme; birleşmenin
  **APK üretmemesi**. Bu işlem APK onayı anlamına gelmez.
- Gerçek DWG doğrulama manifestleri ve kabul kontrol listeleri; gerçek
  cihaz testlerini APK olmadan yapabildiğimiz ölçüde ilerletme.

## Son APK öncesi onay ve güvenlik

1. Özellikle tam AutoCAD 2026 işlevsel eşdeğerlik için planlanan test
   ve mühendislik kontrollerini tamamlayıp durumu kullanıcıya açıkla.
2. Bir kez daha kullanıcıdan **APK üretmeye yönelik açık son onayı** al.
   `main`e merge, testlerin geçmesi, "devam et" ve eski genel izinler
   tek başına APK üretme onayı değildir.
3. Yalnızca bu son onaydan sonra ilgili manuel GitHub
   `workflow_dispatch` yolunu ve `release_approved=true` seçeneğini
   değerlendir; kullanıcı istemedikçe otomatik imzalama, yayımlama,
   Play dağıtımı veya Cloudflare dağıtımı yapma.
4. Eski **Qwen 24 saatlik pilot APK** akışı da farklı bir istisna değil:
   `build_pilot_apk=true` yanı sıra `final_apk_approved=true`
   onayı gerektirir. İki anahtarın varlığı kullanıcı konuşmasındaki
   son açık onayın yerine geçmez.
5. `tests/test_final_apk_hold_gate.py` tüm `.github/workflows/*.yml`
   dosyalarında Gradle `assemble`/`bundle` APK/AAB yollarının
   yukarıdaki kapılarla korunduğunu otomatik kontrol eder.

## Kısıt

Bu kontrol, depo içindeki GitHub iş akışlarını denetler. Kullanıcının
yerel bilgisayarındaki manuel Gradle komutlarını veya GitHub yönetici
yetkileriyle kaynak dışı müdahaleleri teknik olarak engelleyemez.
Ayrıca `assemble` çalıştırmayan derleme testlerini veya kaynak
kodu arşivlerini APK saymaz. Nihai kullanıcı onayı yine ayrıca
gereklidir.
