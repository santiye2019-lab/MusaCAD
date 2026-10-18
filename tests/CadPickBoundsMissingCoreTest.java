import com.musa.cad.CadEdit;
import com.musa.cad.CadPickBounds;
/** Android CAD geometry must not silently continue with a duplicate engine. */
public final class CadPickBoundsMissingCoreTest {
    public static void main(String[] args){
        System.setProperty("java.vm.name","Dalvik");
        boolean blocked=false;
        try {
            CadPickBounds.mightHit(CadEdit.circle(0,0,10,0),10,0,.01f);
        } catch(LinkageError expected){blocked=true;}
        if(!blocked)throw new AssertionError("Missing native library was ignored");
        System.out.println("PASS: missing portable CAD core fails closed");
    }
}
