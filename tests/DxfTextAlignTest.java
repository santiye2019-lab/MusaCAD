import com.musa.cad.DxfTextAlign;

public class DxfTextAlignTest {
    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.001f)throw new AssertionError(name+": "+actual+" != "+expected);}
    private static void point(DxfTextAlign.Result r,float x,float y){near(r.x,x,"x");near(r.y,y,"y");}
    private static void offset(int attachment,float x,float y){float[]v=DxfTextAlign.mtextOffset(attachment,10,-20,110,40);near(v[0],x,"attachment "+attachment+" x");near(v[1],y,"attachment "+attachment+" y");}
    public static void main(String[] args){
        DxfTextAlign.Result left=DxfTextAlign.resolve(10,20,0,0,false,0,0,30,100,20);point(left,10,20);near(left.localOffsetX,0,"left offset");near(left.angleDegrees,30,"left angle");
        DxfTextAlign.Result center=DxfTextAlign.resolve(0,0,50,60,true,1,0,0,100,20);point(center,50,60);near(center.localOffsetX,-50,"center offset");
        DxfTextAlign.Result right=DxfTextAlign.resolve(0,0,50,60,true,2,3,0,100,20);near(right.localOffsetX,-100,"right offset");near(right.localOffsetY,-20,"top offset");
        DxfTextAlign.Result middle=DxfTextAlign.resolve(0,0,50,60,true,4,0,0,100,20);near(middle.localOffsetX,-50,"middle x");near(middle.localOffsetY,-10,"middle y");
        DxfTextAlign.Result aligned=DxfTextAlign.resolve(10,20,210,20,true,3,0,15,100,20);point(aligned,10,20);near(aligned.angleDegrees,0,"aligned angle");near(aligned.widthScale,2,"aligned width");near(aligned.heightScale,2,"aligned height");
        DxfTextAlign.Result fit=DxfTextAlign.resolve(10,20,10,220,true,5,0,0,100,20);near(fit.angleDegrees,90,"fit angle");near(fit.widthScale,2,"fit width");near(fit.heightScale,1,"fit height");
        DxfTextAlign.Result fallback=DxfTextAlign.resolve(5,6,0,0,false,1,2,12,100,20);point(fallback,5,6);near(fallback.localOffsetX,-50,"fallback center");near(fallback.localOffsetY,-10,"fallback vertical");

        offset(1,-10,-40);offset(2,-60,-40);offset(3,-110,-40);
        offset(4,-10,-10);offset(5,-60,-10);offset(6,-110,-10);
        offset(7,-10,20);offset(8,-60,20);offset(9,-110,20);
        float[]fallbackAttachment=DxfTextAlign.mtextOffset(0,10,-20,110,40);near(fallbackAttachment[0],-10,"attachment fallback x");near(fallbackAttachment[1],-40,"attachment fallback y");
        System.out.println("DXF TEXT/MTEXT alignment cases passed");
    }
}
