import com.musa.cad.CadEdit;

public class CadEditStylusTest {
    public static void main(String[] args){
        CadEdit e=CadEdit.freehand(new float[]{1f,2f,3f,4f,5f,6f},6.25f);
        if(e.type!=CadEdit.Type.POLYLINE)throw new AssertionError("freehand must export as polyline");
        if(e.xy.length!=6)throw new AssertionError("freehand points lost");
        if(Math.abs(e.strokeWidth-6.25f)>.001f)throw new AssertionError("pressure width lost");
        CadEdit copy=e.copy();
        e.xy[0]=99f;
        if(copy.xy[0]!=1f)throw new AssertionError("copy must isolate stylus geometry");
        if(Math.abs(copy.strokeWidth-6.25f)>.001f)throw new AssertionError("copy must preserve stylus width");
        System.out.println("Stylus freehand edit persistence passed");
    }
}
