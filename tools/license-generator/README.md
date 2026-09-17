# MusaCAD Lisans Üretici

MusaCAD lisansları çevrimdışı RSA/SHA-256 dijital imza ile doğrulanır.

## Güvenlik modeli
- APK içinde yalnız `MUSACAD-LICENSE-PUBLIC.pem` bulunur.
- Özel anahtar yalnız lisans üretici bilgisayarda tutulur.
- Özel anahtar GitHub'a, APK'ya, e-postaya veya müşteriye verilmez.
- Lisans kodu MusaCAD'de gösterilen **Cihaz / Lisans Kimliği** için üretilir.
- Süreli veya süresiz lisans üretilebilir.

## Masaüstü arayüzü
```bash
javac -d out ../../app/src/main/java/com/musa/cad/LicenseToken.java LicenseGeneratorApp.java
java -cp out LicenseGeneratorApp
```

## Komut satırı
Yeni anahtar çifti üretmek için:
```bash
java -cp out LicenseGenerator keygen MusaCAD-private.pem MusaCAD-public.pem
```

Lisans üretmek için:
```bash
java -cp out LicenseGenerator issue MusaCAD-private.pem <CIHAZ-KIMLIGI> 365
java -cp out LicenseGenerator issue MusaCAD-private.pem <CIHAZ-KIMLIGI> perpetual
```

> Uygulamadaki public key ile üreticide kullanılan private key aynı anahtar çiftine ait olmalıdır. Public key değişirse MusaCAD APK yeniden derlenmelidir.
