import com.musa.cad.MusaAiYfkCatalogQuery;

public final class MusaAiYfkCatalogQueryTest {
    private static void check(boolean yes,String message){if(!yes)throw new AssertionError(message);}
    public static void main(String[]args){
        String official=MusaAiYfkCatalogQuery.SOURCE_URL;
        check(MusaAiYfkCatalogQuery.isOfficialUrl(official),"Pinned official YFK URL");
        check(!MusaAiYfkCatalogQuery.isOfficialUrl(
            "https://webdosya.csb.gov.tr.evil.test/v2/yfk/2026/01/x.pdf"),
            "Lookalike URL must fail");
        check(!MusaAiYfkCatalogQuery.isOfficialUrl(
            "http://webdosya.csb.gov.tr/v2/yfk/2026/01/x.pdf"),
            "HTTP must fail");
        check(!MusaAiYfkCatalogQuery.isOfficialUrl(
            "https://user@webdosya.csb.gov.tr/v2/yfk/2026/01/x.pdf"),
            "Userinfo cannot override source");
        check(!MusaAiYfkCatalogQuery.isOfficialUrl(
            "https://webdosya.csb.gov.tr/v2/yfk/2025/01/x.pdf"),
            "No 2025 substituted for 2026");
        check(MusaAiYfkCatalogQuery.wantsDownload("ÇŞB kitabını indir"),
            "Natural language offline catalog install");
        check(MusaAiYfkCatalogQuery.wantsDownload("YFK arşivi kur"),
            "Synonym archive install");
        check(!MusaAiYfkCatalogQuery.wantsDownload("ÇŞB fiyat listesi yükle"),
            "Curated YFK price CSV picker must not be hijacked");
        check(MusaAiYfkCatalogQuery.wantsStatus("ÇŞİDB katalog durumu"),
            "Offline catalog status");
        check(MusaAiYfkCatalogQuery.lookup("Poz 25.100.1005 fiyatı").equals("25.100.1005"),
            "Exact published position number is preserved");
        check(MusaAiYfkCatalogQuery.lookup("ÇŞB kitabında PVC boru ara").equals("pvc boru"),
            "Full text title query");
        check(MusaAiYfkCatalogQuery.lookup("Merhaba").isEmpty(),
            "Unrelated conversational intent is not hijacked");
        check(MusaAiYfkCatalogQuery.safeSnippet(
            "Başlangıç\\n25.100.1005 Ad 1.234,56\\nBitiş","25.100.1005")
            .contains("1.234,56"),"Reader presents nearby original price text verbatim");
        check(MusaAiYfkCatalogQuery.safeSnippet("PVC Boru 10m","bakır").isEmpty(),
            "No invented unrelated snippets");
        check(MusaAiYfkCatalogQuery.PRICE_PERIOD.equals("2026-01"),
            "Annual edition must NOT silently pretend 2026-10 prices");
        System.out.println("MusaAiYfkCatalogQueryTest OK");
    }
}
