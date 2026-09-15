import com.musa.cad.DxfVisibility;

public class DxfVisibilityTest {
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void no(boolean v,String m){if(v)throw new AssertionError(m);}
    public static void main(String[] args){
        yes(DxfVisibility.invisible("LINE",1,0),"common hidden");
        no(DxfVisibility.invisible("LINE",0,1),"LINE group70 is not attribute visibility");
        yes(DxfVisibility.invisible("ATTRIB",0,1),"invisible ATTRIB");
        yes(DxfVisibility.invisible("attdef",0,5),"invisible ATTDEF plus other flags");
        no(DxfVisibility.invisible("ATTRIB",0,2),"constant visible ATTRIB");
        no(DxfVisibility.invisible("ATTDEF",0,8),"preset visible ATTDEF");
        System.out.println("6 DXF entity visibility cases passed");
    }
}
