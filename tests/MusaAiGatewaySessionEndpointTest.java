import com.musa.cad.*;

public final class MusaAiGatewaySessionEndpointTest {
    private static void eq(String a,String b){if(!a.equals(b))throw new AssertionError(a+" != "+b);}
    public static void main(String[]args){
        eq(MusaAiGatewaySessionClient.sessionEndpoint("https://ai.example.com/analyze"),"https://ai.example.com/session");
        eq(MusaAiGatewaySessionClient.sessionEndpoint("https://ai.example.com/v1/analyze?x=1"),"https://ai.example.com/v1/session");
        eq(MusaAiGatewaySessionClient.sessionEndpoint("https://ai.example.com/api"),"https://ai.example.com/session");
        System.out.println("MusaAiGatewaySessionEndpointTest OK");
    }
}
