import com.musa.cad.LockedScreenLayout;

public class LockedScreenLayoutTest {
    public static void main(String[] args){
        check(1080,2400,600f,1535f);
        check(1440,3200,600f,1535f);
        check(1080,1920,600f,1535f);
        check(720,1600,600f,1535f);
        check(2200,1080,600f,1535f);

        LockedScreenLayout.Size bad=LockedScreenLayout.fit(0,0,600f,1535f);
        if(bad.width!=1||bad.height!=1)throw new AssertionError("invalid bounds fallback");

        int text1080=LockedScreenLayout.scalePx(938,600f,25f);
        int text720=LockedScreenLayout.scalePx(625,600f,25f);
        if(text1080<=text720)throw new AssertionError("larger fitted stage must produce larger overlay text");
        if(LockedScreenLayout.scalePx(600,600f,25f)!=25)throw new AssertionError("reference pixel identity");
        if(LockedScreenLayout.scalePx(0,600f,25f)!=1)throw new AssertionError("invalid scale fallback");

        System.out.println("Locked screen aspect-fit and responsive overlay scale cases passed");
    }

    private static void check(int rootW,int rootH,float artW,float artH){
        LockedScreenLayout.Size s=LockedScreenLayout.fit(rootW,rootH,artW,artH);
        if(s.width<=0||s.height<=0||s.width>rootW||s.height>rootH)throw new AssertionError("bounds");
        double source=artW/artH;
        double actual=(double)s.width/s.height;
        if(Math.abs(source-actual)>.0025)throw new AssertionError("aspect ratio changed: "+actual+" != "+source);
        boolean touchesWidth=Math.abs(s.width-rootW)<=1;
        boolean touchesHeight=Math.abs(s.height-rootH)<=1;
        if(!touchesWidth&&!touchesHeight)throw new AssertionError("stage must fill at least one screen axis");
    }
}
