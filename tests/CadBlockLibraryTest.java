import com.musa.cad.*;
import java.util.*;

public class CadBlockLibraryTest {
    public static void main(String[] args){
        CadBlockLibrary blocks=new CadBlockLibrary();
        if(!blocks.define("pompa",CadEdit.line(0,0,10,0)))throw new AssertionError("define");
        if(!blocks.contains("POMPA")||blocks.names().size()!=1)throw new AssertionError("name lookup");
        List<CadEdit> inserted=blocks.insert("pompa",100,50,2f,90f);
        if(inserted.size()!=1)throw new AssertionError("insert count");
        CadEdit line=inserted.get(0);
        near(line.centerX(),100,"insert center x");near(line.centerY(),50,"insert center y");
        float length=(float)Math.hypot(line.xy[2]-line.xy[0],line.xy[3]-line.xy[1]);
        near(length,20,"insert scale");
        float dx=Math.abs(line.xy[2]-line.xy[0]),dy=Math.abs(line.xy[3]-line.xy[1]);
        if(dx>.02f||Math.abs(dy-20f)>.02f)throw new AssertionError("insert rotation");
        System.out.println("Block definition/insert transform cases passed");
    }
    private static void near(float a,float b,String name){if(Math.abs(a-b)>.02f)throw new AssertionError(name+": "+a+" != "+b);}
}
