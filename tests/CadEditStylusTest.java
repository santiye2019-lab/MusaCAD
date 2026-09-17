import com.musa.cad.CadEdit;

public class CadEditStylusTest {
    public static void main(String[] args){
        float[] source={1f,2f,3f,4f,5f,6f};
        CadEdit e=CadEdit.freehand(source,6.25f);
        if(e.type!=CadEdit.Type.POLYLINE)throw new AssertionError("freehand must export as polyline");
        if(e.xy.length!=source.length)throw new AssertionError("freehand points lost");
        if(Math.abs(e.strokeWidth-6.25f)>.001f)throw new AssertionError("pressure width lost");
        source[0]=99f;
        if(e.xy[0]==99f)throw new AssertionError("freehand must clone input geometry");
        CadEdit copy=e.copy();
        e.xy[0]=77f;
        if(copy.xy[0]!=1f)throw new AssertionError("copy must isolate stylus geometry");
        if(Math.abs(copy.strokeWidth-6.25f)>.001f)throw new AssertionError("copy must preserve stylus width");
        CadEdit floor=CadEdit.freehand(new float[]{0f,0f,1f,1f},0f);
        if(floor.strokeWidth<1f)throw new AssertionError("stylus width floor missing");
        System.out.println("Stylus freehand edit persistence passed");
    }
}
