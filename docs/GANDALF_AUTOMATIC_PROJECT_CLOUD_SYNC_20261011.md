# MusaCAD Gandalf — otomatik proje yükleme, çoklu açık proje koruması

**Kullanıcı tercihi, 11.10.2026:** Açılan her DWG/DXF projesi için tekrar izin penceresi açılmadan, sunucuya otomatik aktarım yapılacak. Aynı anda açılan farklı disiplin çizimleri sunucuda birlikte tutulacak. Bu ayar ilk kez kullanıcı tarafından açıkça talep edilmiştir; kullanıcıya **Otomatik proje aktarımı** için kapatma ve sunucu kopyalarını temizleme kontrolü sunulmalıdır.

**Bu dalın durumu:** Retention (saklama/temizleme) kuralı ve birim testleri eklendi. Bu belge mimari sözleşmedir. **Gerçek otomatik yükleme, R2 kurulumu, Android bağlantısı ve canlı veri aktarımı henüz uygulanmadı.** Sadece retention yordamının bulunması çalışan yükleme servisi anlamına gelmez.

## Kullanıcı davranışı

1. MusaCAD açıldığında gizli anahtar içermeyen `GET /health` ve arka planda imzalı AI session hazır edilir. Sağlık kontrolü modelin görsel yorumlayabildiğinin kanıtı değildir.
2. Kullanıcı proje dosyası açtığında, yerel açılış/rendeleme engellenmeden arka plan aktarımı başlar. Dosya kullanıcıdan her defasında yeniden izin istenmeden senkronize edilir.
3. **Kaynak projenin tamamı** ile CAD varlık kimlikleri, katmanlar, koordinatlar, yazılar/ölçü birimi, layout ve gerektiğinde kontrollü pafta görüntüleri ayrı doğrulanabilir içerikler olarak hazırlanır. İlk 5.000 nesne veya bir görsel ızgara asla %100 *mühendislik kapsaması* diye gösterilmez.
4. Dosya paketleri HTTPS üzerinden sınırlı boyutta ve devam ettirilebilir parçalar olarak yüklenir. Telefon `aktarılmış/onaylanmış bayt ÷ toplam bayt` üzerinden yüzde gösterir. İşlem sonunda sunucu manifestini ve parça özetlerini doğrular; ancak ondan sonra **Aktarım %100** ve **İncele ve Raporla** etkin olur.
5. Görsel/sayısal analiz, gönderim tamamlandıktan sonra kullanıcının **İncele ve Raporla** komutuyla başlar. Analiz iş ilerleme yüzdesi ayrı tutulur; model talep/kota sınırları devam eder.
6. Aynı anda açık A/B/C/D projeleri aynı cihazın sunucu oturumunda birlikte korunur. Aktif proje sekmesi değişikliği saklama kümesini değiştirmez. Zaten açık dosya yeniden seçildiğinde ikinci kez kopyalanmaz.
7. Açık bir proje kapatıldığında ve en az bir başka tam aktarılmış açık proje kaldığında, kapatılan sunucu kopyası silinebilir. **Tüm projeler kapatıldıysa son doğrulanmış kopya bir sonraki başarılı aktarım tamamlanana kadar geçici tutulur.**
8. Yeni D açıldığında A daha önce kapatılmış ve eski başarılı kopya olarak duruyorsa, D tamamen doğrulanmadan A silinmez. D başarılı aktarılınca A silinir. Diğer açık disiplinlerin tamamı korunur.
9. Silme sadece sunucunun proje önbelleğini kapsar. Telefonun orijinal DWG/DXF dosyası, yerel çizim düzenlemesi, yakın geçmiş veya recovery dosyası bu temizleme ile **silinmez**.
10. İnternet kesildiğinde parça yüklemesi kaldığı yerden sürdürülür ve aktarım ilerlemesi doğrulanmadan %100 gösterilmez. Kullanıcı yerelde çizime devam edebilir. Başarısız aktarım raporda bulut denetimi yapılmış gibi gösterilmez.

## Sunucu bileşenleri ve güvenlik

- Geçerli `MAI1/MAI2` imzalı oturum olmadan saklama API'si erişilemez. Sunucu tarafındaki **her** işlem, token içindeki cihaz kimliğine ait ayrı alanda yetkilendirilir. Android veya URL tarafından gönderilen başka bir cihaz kimliği güvenilir sayılmaz.
- Cloudflare Worker kalıcı disk değildir. Özel `MUSACAD_PROJECT_BUCKET` adlı **private R2 binding** veya eşdeğer kalıcı depo gereklidir. Cloudflare erişimi, yapılandırılmış/bütçelenmiş bir bucket ve başarıyla çalışan lisans kontrolü olmadan otomatik yükleme etkinleştirilmez.
- Önerilen API: `POST /v1/projects/init`, `PUT /v1/projects/:projectId/chunks/:part`, `POST /v1/projects/:projectId/complete`, `POST /v1/projects/sync-open`, `GET /v1/projects/status`. Hepsi HTTPS ve yetkilendirmeli; parça işlemleri idempotent ve her paket SHA-256 ile doğrulanmış olmalı.
- Proje UUID'si her açık proje oturumu için benzersizdir, mevcut tekil kaynak dosyayı tekrar aktive etmek aynı UUID'yi korur.
- Proje manifesti proje adı, veri bileşeni (kaynak DWG/DXF, tam CAD indeksi, görsel bölge vb.), parça sayısı, toplam bayt, SHA-256, şema sürümü ve son doğrulama zamanını taşır.
- Her iki yazma işlemi için aynı cihazın proje kataloğunda **seri hale getirme** (Durable Object veya güvenli CAS/versiyon mekanizması) gerekir. Sıra dışı gelen eski açık-sekme revizyonları işlenmemelidir; çok geç ulaşan kapatma bildirimi yeni açılmış bir projeyi silemez.
- Sunucu önce yeni manifestin doğruluğunu ve kalıcılığını onaylar, ardından kapalı kopyaları siler; kısmi yüklemede asla yıkıcı temizlik yapılmaz. Temizlik başarısızsa yeniden deneme kuyruğu ile sınırlı bir gecikmeyle tamamlanır. Bir silme hatası yeni veri kaybına yol açmamalı.
- Kullanıcı değişiklik yaptığında proje sürümünü ayrıştır; eski ve yeni parçaların karışmasına izin verme. Yeni sürüm doğrulanana kadar eski tam yükleme korunur.
- Açık proje sayısı mevcut Android sınırı olarak **4**; en büyük aktarım/kullanıcı bütçesi ve tamamlanmayan paketlerin yaşam süresi sunucuda zorunlu.
- R2 yalnız özel bucket olarak yapılandırılmalı; kamuya açık bucket URL'si olmamalı. Yükleme yapan cihazın dışındaki kişilerden ve başkalarının proje namespace'inden erişim engellenmeli. Aktarım TLS, saklama şifrelemesi, kayıt saklama süresi ve silme/erişim denetimi belgelenmeli.
- Yanıt ve loglarda cihaz/lisans kimliği, görüntü içeriği, ham CAD JSON, DWG bytes, token veya dosya adının gizli bölümleri yazdırılmamalı.
- Kullanıcı açık izin tercihini gelecekte kapatabilmeli, uygulama da gizli arka plan yüklemesi varmış gibi davranmamalı. Belediye/işveren proje paylaşım yetkisi konusunda gerekli kurum politikaları ayrıca dikkate alınmalı.

## Saklama algoritması

`server/ai-worker/src/project-retention.js` yalnızca tam doğrulanmış projeler ve telefondan son doğrulanmış *açık proje UUID kümesi* üzerinden `keepProjectIds` ve `deleteProjectIds` üretir.

- Her tamamlanmış **açık** proje korunur.
- Henüz yüklenmeyen açık projeler `needsPendingUpload=true` ile gösterilir ve **başarılı sayılmaz**.
- Henüz hiçbir açık projenin tam kopyası yoksa son başarılı eski kopya korunur.
- Hiç açık proje yoksa en son doğrulanmış kopya geçici saklanır.
- Temizleme yalnızca en güncel revizyon kalıcı kayda geçtikten sonra yapılır.

## Kabul testleri

- Tek proje A: %100 yüklenmiş, indirmek/çizmek mümkün, diğer kullanıcı erişemez.
- Aynı anda A+B+C: üçünün de tam kopyası sunucuda; sekme geçişi bu durumu bozmaz.
- B kapanır, A+C açık: yalnız B kopyasının silinmesi; A+C korunur.
- A kapanır, D yüklemesi %35'ta kopar: eski A, D doğrulanıncaya kadar kaybolmaz (tek hazır kopya ise).
- D %100 tamamlanır: kapalı A silinir; diğer açık projeler korunur.
- Sunucu 413/429/503, zaman aşımı, paket SHA uyumsuzluğu veya yetkisiz istek döndürürse yanlış %100 veya çizim silinmesi yok.
- Analiz %100 ancak gerçekten incelenen paftalar/bölgeler kanıtlandıysa söylenir; %100 **dosya aktarımı** ve %100 **mühendislik incelemesi** farklı durumdur.
- Uygulama kapatılıp geri açıldığında yeni oturumun açık proje revizyonu, önceki eski eşzamanlı bildirimlerden daha güçlüdür.
