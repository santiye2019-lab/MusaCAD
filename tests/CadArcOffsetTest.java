import com.musa.cad.CadEdit;

public final class CadArcOffsetTest {
    private static void check(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    private static void close(float a,double b,String why){
        if(Math.abs(a-b)>1e-4)throw new AssertionError(why+": "+a+" vs "+b);
    }
    private static void shape(CadEdit arc,float radius,String why){
        check(arc!=null&&arc.type==CadEdit.Type.ARC,why+" type");
        close(arc.centerX(),0,why+" center X");
        close(arc.centerY(),0,why+" center Y");
        for(int i=0;i<6;i+=2){
            double distance=Math.hypot(arc.xy[i]-arc.centerX(),arc.xy[i+1]-arc.centerY());
            if(Math.abs(distance-radius)>1e-4)
                throw new AssertionError(why+" point radius mismatch: "+distance);
        }
        check(arc.xy[1]==0 && arc.xy[2]==0 && arc.xy[5]==0,why+" cardinal shape");
        check(arc.xy[0]>0&&arc.xy[3]>0&&arc.xy[4]<0,why+" preserved CCW sweep");
    }
    public static void main(String[] args){
        CadEdit original=CadEdit.arc(5,0,0,5,-5,0);
        check(original!=null,"source semicircle");
        CadEdit source=original.withLayerOverride("MEKANIK-BORU");
        CadEdit outer=source.offset(2f);
        shape(outer,7,"outward");
        CadEdit inner=source.offset(-1.5f);
        shape(inner,3.5f,"inward");
        check("MEKANIK-BORU".equals(outer.layerOverride) &&
              "MEKANIK-BORU".equals(inner.layerOverride),"source discipline layer must survive offset");
        check(source.xy[0]==5f && source.xy[3]==5f &&
              "MEKANIK-BORU".equals(source.layerOverride),
              "offset cannot mutate original arc");
        check(source.offset(-5)==null && source.offset(-6)==null,
              "zero or negative resulting radius must be rejected");
        check(source.offset(Float.NaN)==null && source.offset(Float.POSITIVE_INFINITY)==null &&
              source.offset(0f)==null,"invalid distances must be rejected");
        CadEdit reversed=CadEdit.arc(-5,0,0,-5,5,0);
        check(reversed!=null,"clockwise input");
        CadEdit shifted=reversed.offset(1f);
        check(shifted!=null && shifted.xy[1]==0f && shifted.xy[3]<0f &&
              shifted.xy[4]>0f,"clockwise orientation preserved");
        System.out.println("CAD concentric ARC OFFSET: signed radius, sweep, layer and source safety PASS");
    }
}
