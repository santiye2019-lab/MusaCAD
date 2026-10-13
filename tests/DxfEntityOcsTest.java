import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** DXF TRACE/SOLID are OCS; 3DFACE and LINE are WCS. */
public final class DxfEntityOcsTest {
    private static String tags(Object...values){
        StringBuilder out=new StringBuilder();
        for(Object value:values)out.append(value).append('\n');
        return out.toString();
    }
    private static String doc(String blocks,String entities){
        return tags(0,"SECTION",2,"BLOCKS")+blocks
            +tags(0,"ENDSEC",0,"SECTION",2,"ENTITIES")
            +entities+tags(0,"ENDSEC",0,"EOF");
    }
    private static String triangle(String type,Object...extra){
        return tags(0,type,10,1,20,2,30,3,11,2,21,2,31,3,12,1,22,3,32,3)+tags(extra);
    }
    private static Dxf3dMesh parse(String dxf)throws Exception{
        File file=File.createTempFile("musacad-entity-ocs-", ".dxf");
        try{
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            return Dxf3dMesh.read(file);
        }finally{Files.deleteIfExists(file.toPath());}
    }
    private static void near(float got,double expected,String what){
        if(Math.abs(got-expected)>1e-4)
            throw new AssertionError(what+" got "+got+" expected "+expected);
    }
    private static void xyz(Dxf3dMesh mesh,int i,double x,double y,double z){
        int n=i*3;
        near(mesh.xyz[n],x,"x"+i);
        near(mesh.xyz[n+1],y,"y"+i);
        near(mesh.xyz[n+2],z,"z"+i);
    }
    private static void tiltedTrace()throws Exception{
        Dxf3dMesh m=parse(doc("",triangle("TRACE",210,1,220,0,230,0)));
        if(m.xyz.length!=9||m.triangles.length!=3)
            throw new AssertionError("TRACE triangle geometry");
        // Extrusion +X: OCS X=(0,1,0), Y=(0,0,1), Z=(1,0,0).
        xyz(m,0,3,1,2);xyz(m,1,3,2,2);xyz(m,2,3,1,3);
        for(int line:m.sourceLines)if(line!=-1)
            throw new AssertionError("tilted OCS coordinates must not be written as WCS");
        // A non-unit normal must normalize to the same basis.
        Dxf3dMesh scaled=parse(doc("",triangle("TRACE",210,10,220,0,230,0)));
        xyz(scaled,0,3,1,2);
    }
    private static void negativeSolid()throws Exception{
        String solid=triangle("SOLID",13,2,23,3,33,4,210,0,220,0,230,-10);
        Dxf3dMesh m=parse(doc("",solid));
        if(m.xyz.length!=12||m.triangles.length!=6)
            throw new AssertionError("SOLID quad geometry");
        xyz(m,0,-1,2,-3);xyz(m,1,-2,2,-3);xyz(m,3,-2,3,-4);
        for(int line:m.sourceLines)if(line!=-1)
            throw new AssertionError("negative OCS must not permit direct WCS source edits");
    }
    private static void nestedTilt()throws Exception{
        String block=tags(0,"BLOCK",2,"T",10,0,20,0,30,0)
            +triangle("TRACE",210,1,220,0,230,0)+tags(0,"ENDBLK");
        String insert=tags(0,"INSERT",2,"T",10,0,20,0,30,0,210,0,220,1,230,0);
        Dxf3dMesh m=parse(doc(block,insert));
        // The TRACE OCS +X is composed inside the INSERT OCS +Y.
        xyz(m,0,-3,2,1);xyz(m,1,-3,2,2);
        for(int line:m.sourceLines)if(line!=-1)
            throw new AssertionError("nested OCS must remain source read-only");
    }
    private static void wcsFaceUnaffected()throws Exception{
        Dxf3dMesh m=parse(doc("",triangle("3DFACE",210,1,220,0,230,0)));
        xyz(m,0,1,2,3);xyz(m,1,2,2,3);
        if(m.sourceLines[0]<0)
            throw new AssertionError("direct WCS 3DFACE must remain editable");
    }
    private static void defaultOcsEditable()throws Exception{
        Dxf3dMesh m=parse(doc("",triangle("TRACE")));
        xyz(m,0,1,2,3);
        if(m.sourceLines[0]<0)
            throw new AssertionError("default +Z OCS TRACE must remain editable");
    }
    private static void invalidNormalFails()throws Exception{
        try{
            parse(doc("",triangle("SOLID",210,0,220,0,230,0)));
            throw new AssertionError("zero extrusion normal must not be silently rendered");
        }catch(IOException expected){
            if(!expected.getMessage().contains("OCS"))
                throw new AssertionError("invalid OCS must be identified",expected);
        }
    }
    public static void main(String[] args)throws Exception{
        tiltedTrace();
        negativeSolid();
        nestedTilt();
        wcsFaceUnaffected();
        defaultOcsEditable();
        invalidNormalFails();
        System.out.println("TRACE/SOLID entity OCS: tilted, nested, negative, WCS parity and source protection OK");
    }
}
