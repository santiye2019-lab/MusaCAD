import com.musa.cad.Cad3dOnlyOpenGuard;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

public final class Cad3dOnlyOpenGuardTest {
    private static void yes(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    private static File fixture(String body)throws Exception {
        File file=File.createTempFile("musacad-3d-route-", ".dxf");
        Files.writeString(file.toPath(),body,StandardCharsets.UTF_8);
        return file;
    }
    private static void reject(Dxf3dMesh mesh,String what)throws Exception {
        boolean thrown=false;
        try{Cad3dOnlyOpenGuard.requireRealGeometry(mesh);}
        catch(IOException expected){thrown=true;}
        yes(thrown,what);
    }
    public static void main(String[] args)throws Exception {
        // Legitimate 3DFACE-only geometry; never require a 2D drawable.
        File real=fixture("0\nSECTION\n2\nENTITIES\n"+
            "0\n3DFACE\n10\n0\n20\n0\n30\n2\n"+
            "11\n10\n21\n0\n31\n2\n"+
            "12\n0\n22\n10\n32\n2\n"+
            "0\nENDSEC\n0\nEOF\n");
        File proxy=fixture("0\nSECTION\n2\nHEADER\n"+
            "9\n$EXTMIN\n10\n0\n20\n0\n30\n0\n"+
            "9\n$EXTMAX\n10\n10\n20\n10\n30\n10\n"+
            "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
            "0\n3DSOLID\n70\n1\n1\nUNAVAILABLE_SAB_PAYLOAD\n"+
            "0\nENDSEC\n0\nEOF\n");
        File empty=fixture("0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");
        try {
            byte[] original=Files.readAllBytes(real.toPath());
            Dxf3dMesh actual=Dxf3dMesh.read(real);
            Cad3dOnlyOpenGuard.requireRealGeometry(actual);
            yes(actual.xyz.length==9&&actual.triangles.length==3,
                "3D-only triangle should have real geometry");
            yes(Arrays.equals(original,Files.readAllBytes(real.toPath())),
                "preflight must never change source bytes");
            Dxf3dMesh placeholder=Dxf3dMesh.read(proxy);
            yes(placeholder.solidProxy,
                "unavailable solid must not be confused with real SAT surface");
            reject(placeholder,"synthetic extents proxy must be rejected");
            reject(null,"missing mesh must fail safely");
            boolean failed=false;
            try{Dxf3dMesh.read(empty);}
            catch(IOException expected){failed=true;}
            yes(failed,"empty CAD file must not pass 3D-only validation");
        }finally {
            Files.deleteIfExists(real.toPath());
            Files.deleteIfExists(proxy.toPath());
            Files.deleteIfExists(empty.toPath());
        }
        System.out.println("3D-only open guard: real mesh accepted, extents proxy and empty file rejected");
    }
}
