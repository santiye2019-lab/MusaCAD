import com.musa.cad.DxfVisibility;

public class DxfVisibilityTest {
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void no(boolean v,String m){if(v)throw new AssertionError(m);}
    public static void main(String[] args){
        yes(DxfVisibility.invisible("LINE",1,0),"common hidden");
        no(DxfVisibility.invisible("LINE",0,1),"LINE group70 is not attribute visibility");
        yes(DxfVisibility.invisible("ATTRIB",0,1),"invisible ATTRIB");
        no(DxfVisibility.invisible("ATTRIB",0,2),"ATTRIB flag 2 does not hide reference");
        yes(DxfVisibility.invisible("ATTDEF",0,1),"invisible ATTDEF");
        yes(DxfVisibility.invisible("ATTDEF",0,0),"variable ATTDEF template is not drawn");
        yes(DxfVisibility.invisible("attdef",0,8),"preset variable ATTDEF template is not drawn");
        no(DxfVisibility.invisible("ATTDEF",0,2),"constant ATTDEF remains visible");
        no(DxfVisibility.invisible("ATTDEF",0,10),"constant preset ATTDEF remains visible");
        yes(DxfVisibility.invisible("ATTDEF",0,3),"invisible constant ATTDEF stays hidden");
        System.out.println("10 DXF entity visibility cases passed");
    }
}
