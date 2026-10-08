# Gandalf AI Android saha testi — 2026-10-08

Amaç: PR #183 ile eklenen Türkçe 'projeye analiz yap' genel analiz yönlendirmesi ve PR #182 ile eklenen CAD-JSON ANR düzeltmelerinin imzalı direct-release APK'da doğrulanması.

Beklenen kontrol:
- Sesli komut: 'Projeye analiz yap' genel yerel disiplin analizini başlatır.
- 'Bu projeyi derin analiz et' açık onayla bulut analizini başlatır.
- Proje nesne ve katman özetleri doğru aktarılır; komut tanınmadı mesajına düşülmez.
- Büyük DWG üzerinde arayüz yanıt vermeyi sürdürür.
- Analiz raporu Word/PDF çıktısına bağlanır.

Bu dosya, mevcut production workflow'un imzalı APK ve AI sunucu üretimini yeniden çalıştırması için oluşturulmuştur. Gerçek cihaz testi hâlâ gereklidir.
