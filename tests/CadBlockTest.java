import com.musa.cad.*;
import java.util.*;

public final class CadBlockTest {
    private static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    public static void main(String[] args){
        CadEdit source=CadEdit.line(10,20,30,40);
        CadEdit relative=CadBlock.memberRelativeToCenter(source);
        check(Math.abs(relative.centerX())<0.0001f&&Math.abs(relative.centerY())<0.0001f,"block center");
        CadBlock.Definition def=new CadBlock.Definition("TEST_BLOCK",Collections.singletonList(relative));
        check("TEST_BLOCK".equals(def.name),"block name");
        CadEdit insert=CadEdit.insert("TEST_BLOCK",100,200,2f,30f);
        check(insert!=null&&insert.type==CadEdit.Type.INSERT,"insert type");
        check(Math.abs(insert.insertScale()-2f)<0.0001f,"insert scale");
        check("A_B".equals(CadBlock.normalizeName("A B")),"block name normalization");
        System.out.println("CadBlockTest OK");
    }
}
