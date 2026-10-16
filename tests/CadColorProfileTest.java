import com.musa.cad.CadColorProfile;

public final class CadColorProfileTest {
    private static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    public static void main(String[]args){
        CadColorProfile initial=new CadColorProfile(),full=new CadColorProfile();
        for(int i=0;i<100;i++)initial.add(0xFF0000FF);
        for(int i=0;i<50;i++)full.add(0xFFFF0000);
        for(int i=0;i<50;i++)full.add(0xFF00FF00);
        check(initial.likelyFirstPaintMismatch(full),"suspicious blue native paint must be diagnosed");
        check(initial.blueFraction()==1d,"blue profile");
        check(full.unique()==2,"different 2D colors");
        check(initial.safeSummary().contains("dominant=#0000FF"),"privacy-safe color-only summary");
        CadColorProfile benign=new CadColorProfile();
        for(int i=0;i<100;i++)benign.add(0xFF0000FF);
        check(!initial.likelyFirstPaintMismatch(benign),"legitimate all-blue source is not an error");
        CadColorProfile tiny=new CadColorProfile();
        for(int i=0;i<5;i++)tiny.add(0xFF0000FF);
        check(!tiny.likelyFirstPaintMismatch(full),"small samples are insufficient evidence");
        CadColorProfile capped=new CadColorProfile();
        for(int i=0;i<10000;i++)capped.add(i);
        check(capped.sampled()==CadColorProfile.MAX_SAMPLES,"bounded sample must prevent unlimited data");
        check(!initial.safeSummary().contains("DWG"),"no filename in diagnostic");
        CadColorProfile monoDifferent=new CadColorProfile();for(int i=0;i<100;i++)monoDifferent.add(0xFFFF0000);
        check(!initial.likelyFirstPaintMismatch(monoDifferent),"legitimately monochrome but differently colored scenes need real evidence");
        System.out.println("Native/DXF first-paint color profile checks passed");
    }
}
