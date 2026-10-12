import com.musa.cad.CadEdit;
import com.musa.cad.CadPickBounds;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public final class CadPickBoundsTest {
    private static void check(boolean valid,String why){if(!valid)throw new AssertionError(why);}
    public static void main(String[]args){
        ArrayList<CadEdit> cases=new ArrayList<>();
        cases.add(CadEdit.line(-40f,20f,100f,-20f));
        cases.add(CadEdit.rectangle(-40f,-10f,40f,90f));
        cases.add(CadEdit.circle(15f,-20f,45f,-20f));
        cases.add(CadEdit.arc(15f,0f,0f,15f,-15f,0f));
        cases.add(CadEdit.point(100f,100f));
        cases.add(CadEdit.polyline(new float[]{0,0,10,20,35,2,-5,5},true));
        cases.add(CadEdit.hatch(new float[]{0,0,20,0,20,30,0,30},"SOLID",0f,1f));
        cases.add(CadEdit.text(4f,9f,"test"));
        cases.add(CadEdit.insert("TEST",1f,2f,1f,0f));
        cases.add(CadEdit.xline(2f,3f,100f,103f));
        cases.add(CadEdit.ellipse(0f,0f,50f,0f,0f,20f));
        cases.add(CadEdit.line(1_000_000f,1_000_000f,1_000_040f,1_000_080f));
        Random random=new Random(100193);
        for(CadEdit edit:cases){
            check(edit!=null,"fixture");
            for(int i=0;i<2000;i++){
                float x=(random.nextFloat()-.5f)*400f;
                float y=(random.nextFloat()-.5f)*400f;
                float tolerance=.001f+random.nextFloat()*12f;
                float exact=edit.hitDistance(x,y);
                if(exact<=tolerance&&!CadPickBounds.mightHit(edit,x,y,tolerance))
                    throw new AssertionError("Prefilter changed precise pick semantics "+edit.type+" at "+x+","+y);
            }
        }
        check(!CadPickBounds.mightHit(CadEdit.circle(0f,0f,10f,0f),100f,100f,5f),
            "far circle should be rejected");
        check(!CadPickBounds.mightHit(CadEdit.polyline(new float[]{0f,0f,2f,2f,3f,3f}),100f,100f,1f),
            "far polyline should be rejected");
        check(CadPickBounds.mightHit(CadEdit.circle(0f,0f,10f,0f),10f,0f,.01f),
            "circle crossing bounds must be retained");
        check(CadPickBounds.mightHit(CadEdit.text(10f,10f,"T"),11.5f,10f,2f),
            "text anchor with expanded tolerance");
        check(CadPickBounds.mightHit(CadEdit.xline(0f,0f,1f,1f),100000f,100000f,1f),
            "unbounded XLINE must not be clipped");
        check(CadPickBounds.mightHit(CadEdit.point(1f,2f),Float.NaN,2f,1f),
            "invalid coordinate falls back to exact geometry test");
        System.out.println("Conservative selection bounds parity across 2D tools passed");
    }
}
