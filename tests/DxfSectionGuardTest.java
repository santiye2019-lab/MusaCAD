import com.musa.cad.DxfSectionGuard;
import java.io.IOException;
/** Native JVM test: no duplicated Java DXF section semantics. */
public final class DxfSectionGuardTest {
    private static int feed(DxfSectionGuard g,int index,String code,String value)
            throws Exception{
        return g.consume(code,value,index,100);
    }
    private static void bad(DxfSectionGuard g,int index,String code,String value,
                            String part)throws Exception{
        try{
            g.consume(code,value,index,100);
            throw new AssertionError("Invalid DXF section record accepted: "+value);
        }catch(IOException expected){
            if(!expected.getMessage().contains(part))
                throw new AssertionError("Wrong native error: "+expected.getMessage(),expected);
        }
    }
    public static void main(String[]args)throws Exception{
        DxfSectionGuard g=new DxfSectionGuard();
        bad(g,0,"0","ENDSEC","ENDSEC");
        bad(g,0,"0","EOF","ENTITIES");
        int p=0;
        feed(g,p++,"0","SECTION");
        bad(g,p,"0","LINE","SECTION adı");
        feed(g,p++,"2","header");
        feed(g,p++,"0","ENDSEC");
        feed(g,p++,"0","SECTION");
        feed(g,p++,"2","entities");
        int packed=feed(g,p++,"0","LINE");
        if(DxfSectionGuard.code(packed)!=0||
           !DxfSectionGuard.event(packed,DxfSectionGuard.RECORD_BOUNDARY)||
           !DxfSectionGuard.event(packed,DxfSectionGuard.LINE_STARTED))
            throw new AssertionError("Missing LINE start or boundary native event");
        packed=feed(g,p++,"5","A1");
        if(DxfSectionGuard.code(packed)!=5||
           !DxfSectionGuard.event(packed,DxfSectionGuard.HANDLE_AREA))
            throw new AssertionError("Identity section event missing");
        for(String[] xy:new String[][]{{"10","0"},{"20","0"},
            {"11","20"},{"21","5"}})feed(g,p++,xy[0],xy[1]);
        feed(g,p++,"0","ENDSEC");
        feed(g,p++,"0","SECTION");
        bad(g,p,"2","ENTITIES","Çift");
        feed(g,p++,"2","OBJECTS");
        feed(g,p++,"0","DICTIONARY");
        feed(g,p++,"0","ENDSEC");
        feed(g,p++,"0","EOF");
        g.requireFinished();
        bad(g,p,"0","LINE","EOF");
        DxfSectionGuard truncated=new DxfSectionGuard();
        feed(truncated,0,"0","SECTION");
        try{
            truncated.requireFinished();
            throw new AssertionError("Truncated DXF was wrongly accepted");
        }catch(IOException expected){
            if(!expected.getMessage().contains("tamamlanmamış"))throw expected;
        }
        DxfSectionGuard invalid=new DxfSectionGuard();
        feed(invalid,0,"0","SECTION");
        feed(invalid,1,"2","ENTITIES");
        feed(invalid,2,"0","LINE");
        feed(invalid,3,"10","1.25");
        feed(invalid,4,"20","-2e3");
        bad(invalid,5,"11","NaN","sayısal");
        bad(invalid,5,"11","1e999","sayısal");
        bad(invalid,5,"11","0x1p3","sayısal");
        feed(invalid,5,"11","3");
        bad(invalid,6,"0","ENDSEC","koordinatları eksik");
        feed(invalid,6,"21","4");
        feed(invalid,7,"0","ENDSEC");
        feed(invalid,8,"0","EOF");
        invalid.requireFinished();
        System.out.println("PASS: shared C section + LINE coordinate state through real JNI");
    }
}
