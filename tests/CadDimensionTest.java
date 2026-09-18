import com.musa.cad.*;
import java.util.*;

public final class CadDimensionTest {
    private static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    public static void main(String[] args){
        List<CadEdit> linear=CadDimension.linear(0,0,100,0,50,40,100,0,10,6,2);
        check(linear.size()==8,"linear dimension primitive count");
        check(linear.get(2).type==CadEdit.Type.LINE,"dimension line");
        CadEdit text=linear.get(7);
        check(text.type==CadEdit.Type.TEXT,"linear dimension text");
        check("100".equals(text.text),"linear dimension value");

        List<CadEdit> aligned=CadDimension.aligned(0,0,30,40,10,60,50,10,6,3);
        check(aligned.size()==8,"aligned dimension primitive count");
        check("50".equals(aligned.get(7).text),"aligned dimension value");

        check("12.35".equals(CadDimension.format(12.345,2)),"precision format");
        check("12".equals(CadDimension.format(12.0,3)),"trim zeros");
        System.out.println("CadDimensionTest OK");
    }
}
