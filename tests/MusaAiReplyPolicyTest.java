import com.musa.cad.MusaAiReplyPolicy;

public final class MusaAiReplyPolicyTest {
    static void check(boolean ok,String reason) {
        if(!ok)throw new AssertionError(reason);
    }
    public static void main(String[] args) {
        check(MusaAiReplyPolicy.asksModelIdentity(
            "Bağlantı testi. Kullanılan yapay zekâ modelini tek cümleyle söyle."),
            "Qwen connection/model identity test");
        check(MusaAiReplyPolicy.asksModelIdentity("Gandalf hangi modeli kullanıyorsun?"),
            "Informal Turkish model question");
        check(!MusaAiReplyPolicy.asksModelIdentity(
            "Projede kullanılan mimari modelin katlarını analiz et."),
            "Engineering model must not be mistaken for AI identity");
        check(!MusaAiReplyPolicy.asksModelIdentity(
            "Qwen modelinin performansı nasıl?"),
            "Capacity discussion should remain conversational");
        check(!MusaAiReplyPolicy.wantsTakeoff(
            "Bağlantı testi. Kullanılan yapay zekâ modelini tek cümleyle söyle."),
            "A connection check must never append BOQ");
        check(!MusaAiReplyPolicy.wantsTakeoff("Projeyi kontrol et ve yangın hatlarını incele."),
            "Review alone is not a material price request");
        check(MusaAiReplyPolicy.wantsTakeoff(
            "Sıhhi tesisat projesinin metrajını ve ÇŞİDB poz keşfini çıkar."),
            "Explicit quantity and unit-price request");
        check(MusaAiReplyPolicy.wantsTakeoff("Malzeme listesini hazırla."),
            "Material list request");
        String trusted=MusaAiReplyPolicy.verifiedModelAnswer("cloudflare","@cf/qwen/qwen3.8-27b");
        check(trusted.contains("@cf/qwen/qwen3.8-27b"),"Trusted Qwen model identity");
        check(trusted.contains("Cloudflare Workers AI"),"Trusted provider identity");
        check(!trusted.contains("Claude"),"No fabricated model");
        check(MusaAiReplyPolicy.verifiedModelAnswer("","").contains("bulunmuyor"),
            "Missing metadata must not be fabricated");
        System.out.println("MusaAiReplyPolicyTest OK");
    }
}
