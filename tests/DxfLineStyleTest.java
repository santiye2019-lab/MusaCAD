import com.musa.cad.DxfLineStyle;

public class DxfLineStyleTest {
    public static void main(String[] args){
        eq(DxfLineStyle.resolveLinetype("BYLAYER","DASHED","CENTER"),"DASHED","bylayer linetype");
        eq(DxfLineStyle.resolveLinetype("BYBLOCK","DASHED","CENTER"),"CENTER","byblock linetype");
        eq(DxfLineStyle.resolveLinetype("hidden","DASHED","CENTER"),"HIDDEN","explicit linetype");

        eq(DxfLineStyle.resolveLineweight(DxfLineStyle.LW_BYLAYER,50,70,25),50,"bylayer weight");
        eq(DxfLineStyle.resolveLineweight(DxfLineStyle.LW_BYBLOCK,50,70,25),70,"byblock weight");
        eq(DxfLineStyle.resolveLineweight(DxfLineStyle.LW_DEFAULT,50,70,35),35,"default weight");
        eq(DxfLineStyle.resolveLineweight(100,50,70,25),100,"explicit weight");

        DxfLineStyle.Pattern continuous=new DxfLineStyle.Pattern("CONTINUOUS",new double[0],false);
        if(continuous.dash(10,1,1,1)!=null)throw new AssertionError("continuous produced dash");
        DxfLineStyle.Pattern dashed=new DxfLineStyle.Pattern("DASHED",new double[]{.5,-.25,0,-.25},false);
        DxfLineStyle.Dash dash=dashed.dash(10,2,.5,1);
        if(dash==null||dash.intervals.length<4||(dash.intervals.length&1)!=0)throw new AssertionError("dash intervals");
        for(float v:dash.intervals)if(!(v>0f)&&Float.isFinite(v))throw new AssertionError("nonpositive interval");
        near(DxfLineStyle.screenStroke(50),3f,"screen 0.50mm");
        near(DxfLineStyle.printStrokePoints(25),(float)(.25*72/25.4),"print 0.25mm");
        System.out.println("DXF linetype/lineweight cases passed");
    }
    private static void eq(String a,String b,String n){if(!b.equals(a))throw new AssertionError(n+": "+a+" != "+b);}
    private static void eq(int a,int b,String n){if(a!=b)throw new AssertionError(n+": "+a+" != "+b);}
    private static void near(float a,float b,String n){if(Math.abs(a-b)>.01f)throw new AssertionError(n+": "+a+" != "+b);}
}
