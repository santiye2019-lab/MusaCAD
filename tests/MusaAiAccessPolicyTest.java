import com.musa.cad.*;

public final class MusaAiAccessPolicyTest {
    private static void eq(Object a,Object b,String msg){
        if(a==null?b!=null:!a.equals(b))throw new AssertionError(msg+" expected="+b+" actual="+a);
    }
    public static void main(String[]args){
        MusaAiAccessPolicy.Decision noGateway=MusaAiAccessPolicy.decide(false,true,true,true);
        eq(noGateway.mode,MusaAiAccessPolicy.Mode.LOCAL_LIMITED,"gateway");

        MusaAiAccessPolicy.Decision onlyIdentity=MusaAiAccessPolicy.decide(true,true,false,true);
        eq(onlyIdentity.mode,MusaAiAccessPolicy.Mode.LOCAL_LIMITED,"identity is not entitlement");

        MusaAiAccessPolicy.Decision expired=MusaAiAccessPolicy.decide(true,true,true,false);
        eq(expired.mode,MusaAiAccessPolicy.Mode.LOCAL_LIMITED,"expired");

        MusaAiAccessPolicy.Decision cloud=MusaAiAccessPolicy.decide(true,true,true,true);
        eq(cloud.mode,MusaAiAccessPolicy.Mode.CLOUD_GANDALF,"cloud");
        if(!cloud.badge.contains("Gandalf"))throw new AssertionError("cloud badge");

        System.out.println("MusaAiAccessPolicyTest OK");
    }
}
