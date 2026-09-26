import com.musa.cad.RenderPathPolicy;

public class RenderPathPolicyTest {
    private static void expect(boolean expected,boolean actual,String name){
        if(expected!=actual)throw new AssertionError(name+" expected "+expected+" got "+actual);
    }
    public static void main(String[] args){
        expect(true,RenderPathPolicy.useNativeFast(false,true,false,0),"native first paint before vector load");
        expect(false,RenderPathPolicy.useNativeFast(true,true,false,0),"full vector renderer must remain authoritative");
        expect(false,RenderPathPolicy.useNativeFast(false,true,true,0),"truncated native scene");
        expect(false,RenderPathPolicy.useNativeFast(false,true,false,1),"edited drawing");
        expect(false,RenderPathPolicy.useNativeFast(false,false,false,0),"native scene missing");
        System.out.println("Navigation render-path color stability cases passed");
    }
}
