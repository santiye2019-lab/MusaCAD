import com.musa.cad.CadUcsFrame;
import com.musa.cad.CadUcsProfiles;
import java.util.Map;

public final class CadUcsProfilesTest {
    private static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    private static void close(double a,double b){if(!Double.isFinite(a)||Math.abs(a-b)>1e-8)throw new AssertionError(a+" != "+b);}
    private static void xyz(double[] p,double x,double y,double z){
        close(p[0],x);close(p[1],y);close(p[2],z);
    }
    private static void rejectsName(String name){
        try{CadUcsProfiles.validName(name);throw new AssertionError("Invalid name accepted: "+name);}
        catch(IllegalArgumentException expected){}
    }
    public static void main(String[] args){
        CadUcsFrame roof=CadUcsFrame.parse("5,6,7,1,1,0,0,0,1");
        String saved=CadUcsProfiles.put(null," Çatı Eğimi ",roof);
        Map<String,CadUcsFrame> restored=CadUcsProfiles.read(saved);
        check(restored.size()==1&&restored.containsKey("Çatı Eğimi"),"UTF-8 Turkish name not restored");
        CadUcsFrame frame=restored.get("Çatı Eğimi");
        xyz(frame.toLocal(roof.toWorld(2,3,4)[0],roof.toWorld(2,3,4)[1],roof.toWorld(2,3,4)[2]),2,3,4);
        xyz(frame.origin(),5,6,7);

        // Multiple profile names, stable order, overwrite and delete operations.
        String second=CadUcsProfiles.put(saved,"Kesit 01",CadUcsFrame.front());
        check(CadUcsProfiles.read(second).size()==2,"Second UCS missing");
        String replaced=CadUcsProfiles.put(second,"Kesit 01",CadUcsFrame.right());
        check(CadUcsProfiles.read(replaced).size()==2,"Replacing UCS grew entry count");
        xyz(CadUcsProfiles.read(replaced).get("Kesit 01").move(2,2),2,0,0);
        String removed=CadUcsProfiles.remove(replaced,"Kesit 01");
        check(CadUcsProfiles.read(removed).size()==1,"Deletion failed");
        check(CadUcsProfiles.read(removed).containsKey("Çatı Eğimi"),"Deleting another UCS changed this entry");

        // Corruption is isolated; valid presets still open and no unsafe axes are substituted.
        String bad="broken-no-tab\n"
            +"Bad\t0,0,0,1,0,0,2,0,0\n"
            +"Over\t0,0,0,NaN,0,0,0,1,0\n"
            +saved;
        check(CadUcsProfiles.read(bad).size()==1,"Invalid saved axes must be ignored");
        check(CadUcsProfiles.read("x".repeat(8193)).isEmpty(),"Oversized stored data should be ignored");

        rejectsName("");
        rejectsName(" ");
        rejectsName("a\nb");
        rejectsName("x\ty");
        rejectsName("x|y");
        rejectsName("verylongnamedwithwaymorethanthirtytwosymbols");
        check("UCS-1_Test.2".equals(CadUcsProfiles.validName(" UCS-1_Test.2 ")),
            "Common engineering profile names rejected");

        String bounded="";
        for(int i=0;i<CadUcsProfiles.MAX_PROFILES;i++)
            bounded=CadUcsProfiles.put(bounded,"UCS-"+i,CadUcsFrame.world());
        check(CadUcsProfiles.read(bounded).size()==12,"Saved profile quota lost");
        boolean rejected=false;
        try{CadUcsProfiles.put(bounded,"UCS-overflow",CadUcsFrame.world());}
        catch(IllegalArgumentException expected){rejected=true;}
        check(rejected,"13th UCS should fail");
        check(CadUcsProfiles.read(CadUcsProfiles.put(bounded,"UCS-0",CadUcsFrame.front())).size()==12,
            "Replace under full quota must work");
        try{
            CadUcsProfiles.read(saved).put("danger",CadUcsFrame.world());
            throw new AssertionError("Read-only profile map exposed mutable internals");
        }catch(UnsupportedOperationException expected){}
        System.out.println("Named UCS profiles: reload, UTF-8 name, edit/delete, corruption, bounds and quotas passed");
    }
}
