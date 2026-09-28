# MusaCAD License Manager

License Manager, MusaCAD için cihaz-bağlı lisans üretir.

## Lisans türleri

### Güvenli MC1 — ticari kullanım
- RSA SHA-256 imzalıdır.
- Tam **Güvenli Lisans Kimliği** için üretilir.
- Private key APK'ya veya repoya gömülmez.
- MusaCAD yalnız kendi `MUSACAD-LICENSE-PUBLIC.pem` anahtarıyla doğrular.
- License Manager, aktif private key fingerprint'i ile MusaCAD public key fingerprint'i eşleşmiyorsa MC1 üretmez.

### 12 haneli kısa kod — legacy uyumluluk
Kısa kod algoritması uygulama kodundan türetilebilir; güçlü ticari lisanslama olarak kabul edilmez. Validation/debug yapılarında geriye dönük uyumluluk için tutulur. `musacadProductionRelease=true` ile üretilen production MusaCAD release'inde kısa kod aktivasyonu kapalıdır.

## Anahtar oluşturma ve yedek

1. License Manager'da **YENİ ANAHTAR** seç.
2. En az 12 karakter yedek parolası gir.
3. Android dosya seçicisinde `.mlk` yedek dosyasının konumunu seç.
4. RSA anahtarı oluşturulur ve private key PBKDF2-HMAC-SHA256 + AES-256-GCM ile şifrelenerek yedeğe yazılır.
5. Yedek başarıyla yazıldıktan sonra aynı private key Android Keystore tarafından sarılarak cihazda aktif edilir.
6. **PUBLIC KEY** ile PEM public key'i dışa aktar.
7. Bu PEM'i MusaCAD'deki `app/src/main/assets/MUSACAD-LICENSE-PUBLIC.pem` ile değiştir.
8. MusaCAD ve License Manager'ı yeniden build et; fingerprint durumu **HAZIR / EŞLEŞTİ** olmalıdır.

## Kurtarma

Yeni telefonda:
1. License Manager'ı kur.
2. **YEDEKTEN DÖN** seç.
3. `.mlk` dosyasını seç ve yedek parolasını gir.
4. Private/public eşleşmesi kriptografik olarak doğrulanır.
5. Private key yeni cihazın Android Keystore sarma anahtarı altında yeniden saklanır.
6. Fingerprint'in MusaCAD production public key ile eşleştiğini doğrula.

## Kritik kurallar
- `.mlk` yedeğini ve parolasını aynı yerde saklama.
- En az iki ayrı güvenli yedek bulundur.
- Private key'i kaynak koda, GitHub'a, e-postaya veya düz metin dosyasına koyma.
- Yeni RSA anahtarına geçmek, eski public key ile yayınlanmış MusaCAD sürümlerinin yeni lisansları doğrulayamaması anlamına gelir.
- Android APK imza anahtarı ile lisans RSA anahtarı ayrı tutulmalıdır.
