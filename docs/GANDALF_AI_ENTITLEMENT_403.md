# Gandalf Cloud AI — Geliştirici yetkisi ve 403 sorunu

## Kullanıcıdaki kesin hata

`Active cloud-AI entitlement not found` yanıtı, **lisans/trial Worker'ın** `POST /v1/ai/session` uç noktasından, HTTP **403** ile gelir. Bu bir görsel DWG dönüşüm veya 5G/internet hız hatası değildir. Bulut AI modelini çağırmak için henüz aktif ve imzalı bir oturum üretilmemiştir.

`/health` yalnız AI ağ geçidinin çalıştığını doğrular; imzalı lisans, yapay zekâ sağlayıcısı, kullanılabilir kota ve model yanıtı için kanıt sayılmaz.

## Yetkilendirme akışı

- Android `MusaAiSessionService.get()`, cihazın **tam güvenli lisans kimliğini** (`LicenseManager.installationId`), paket adını (`com.musa.cad`) ve varsa yerel imzalı lisans kanıtını sunucuya gönderir.
- Sunucu yalnız **geçerli** sunucu imzalı deneme (`MT1`), ödenmiş lisans (`MC1`) veya doğrulanmış Play yenileme (`MP1`) kanıtını kabul eder; alternatif olarak yöneticinin ayrıca yetkilendirdiği bir geliştirici cihazını tanır.
- Sunucudaki geliştirici izin listesi değişkeni `MUSACAD_DEVELOPER_DEVICE_IDS`. Değeri, tam `MC-...` biçimindeki cihaz kimliklerinin virgülle ayrılmış listesidir. **12 karakterlik kısa seri** bu alan için uygun değildir.
- `MUSACAD_DEVELOPER_DEVICE_IDS` listesine kayıt yapılmadan, Android'in geliştirici ekranına geçmesi veya APK'nin yeniden kurulması bu 403 sorununu çözmez.
- Bu tanımlama yalnız yetkili Cloudflare hesabındaki ilgili **MusaCAD lisans/trial Worker** üzerinde yapılmalıdır; `musacad-ai` görsel analiz Worker'ında değil.
- Cloudflare → Workers & Pages → lisans/trial Worker → Settings → Variables and Secrets bölümünden (arayüz adına göre) mevcut tanımı **koruyarak** tam kimliği ekleyin. Kaydedip dağıttıktan sonra uygulamayı yeniden açıp oturum alın.
- Geliştirici listesi kimliğini GitHub kaynak koduna, herkese açık issue'ya veya sohbet mesajına eklemeyin. Gerçek özel anahtarları veya sunucu secret'larını asla mobil istemciye gömmeyin.

## Güvenli cihaz kimliğini uygulamadan almak

Gandalf panelinde AI LED'ine dokunun. Açılan hata penceresinde **KİMLİĞİ KOPYALA** düğmesi vardır; tam kimlik yalnız kullanıcının kendi Android panosuna kopyalanır, MusaCAD sunucusuna bu işlem nedeniyle ek istek atılmaz. Android 13+ üzerinde pano içeriği hassas olarak işaretlenir.

Alternatif olarak `LicenseActivity` → Güvenli Lisans Kimliği → KOPYALA. Kısa serial ID yerine `LicenseManager.installationId` kullanılmalıdır.

## Güvenlik sınırı / üretime alma notu

Mevcut lisans sunucusunda geliştirici tanıma, gelen cihaz ID'sinin allowlist'e eşleşmesine dayanıyor. Cihaz kimliği tek başına kriptografik cihaz kimliği kanıtı değildir ve sahte bir istemciden taklit edilebilir. Dolayısıyla bu mekanizma bir **uzun vadeli yönetici kimlik doğrulama tasarımı olarak yeterli değildir**. Üretimde, kimliğe ek olarak sunucuda doğrulanan kullanıcı hesabı, imzalı yönetici lisansı veya challenge-response cihaz anahtarı zorunlu tutulmalıdır. Allowlist tüm kullanıcılara açılmamalı; sunucu tarafı kimlik ve kota kontrolleri kaldırılmamalıdır.

## Hata tanısı

| LED / Sunucu yanıtı | Nedeni | Eylem |
|---|---|---|
| AI oturum hatası / HTTP 403 / Active cloud-AI entitlement not found | Geçerli lisans kanıtı yok, cihaz yetkili geliştirici listesinde eşleşmedi | Meşru lisans veya yetkili cihaz kimliğiyle aktivasyon |
| AI kota doldu / 429 | Model sağlayıcı kotası dolu | Kota/plan durumu kontrolü |
| AI zaman aşımı / 504 | Model görsel yanıtı süresinde gelmedi | Daha küçük bölgeyle tekrar deneme |
| Erişim yok | Sağlık bağlantısı başarısız | Bağlantı/gateway doğrulaması |

Önemli: Kullanıcının ekranındaki 0/9 bölge, bulut AI'nın **hiçbir görsel bölgeyi başarıyla incelemediği** anlamına gelir. Yerel CAD ön incelemesini görsel AI projesi uygunluk raporu gibi sunmayın.
