import com.musa.cad.Cad3dDwgHandleCrosscheck;
import com.musa.cad.Cad3dDxfHandleIndex;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** Pure JVM model identity test: NO claim of physical DWG roundtrip. */
public final class Cad3dDwgHandleCrosscheckTest {
    private static void expect(boolean value) {
        if(!value)throw new AssertionError("DWG/DXF cross-check contract");
    }
    private static void reject(long[] data,long first,long next,boolean more)
            throws Exception {
        Cad3dDwgHandleCrosscheck.Builder b=new Cad3dDwgHandleCrosscheck.Builder();
        try {
            b.addPage(first,data,next,more);
            throw new AssertionError("Corrupt native page was accepted");
        } catch(IOException good) {
            try {b.finish();throw new AssertionError("Partial set accepted");}
            catch(IOException expected){}
        }
    }

    public static void main(String[] args) throws Exception {
        File file=File.createTempFile("musacad-native-model-id-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nLINE\n5\n000A1\n10\n0\n20\n0\n11\n1\n21\n1\n"+
                "0\n3DFACE\n5\nB2\n10\n0\n20\n0\n30\n1\n"+
                "11\n1\n21\n0\n31\n1\n12\n1\n22\n1\n32\n1\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            Cad3dDxfHandleIndex idx=Cad3dDxfHandleIndex.read(file);
            Cad3dDwgHandleCrosscheck.Builder b=new Cad3dDwgHandleCrosscheck.Builder();
            b.addPage(0,new long[]{
                0xA1,Cad3dDwgHandleCrosscheck.TYPE_LINE,
                0xB2,Cad3dDwgHandleCrosscheck.TYPE_3DFACE
            },2,true);
            b.addPage(2,new long[]{
                0xCC,Cad3dDwgHandleCrosscheck.TYPE_OTHER
            },3,false);
            Cad3dDwgHandleCrosscheck audit=b.finish();
            expect(audit.modelEntityCount()==3);
            expect(audit.matchesDirectModel(idx.forVertex(mesh,0)));
            expect(audit.matchesDirectModel(idx.forTriangle(mesh,0)));
            expect(!audit.matchesDirectModel(null));
            expect(!audit.matchesDirectModel(idx.forVertex(mesh,-1)));
            expect(!audit.matchesDirectModel(idx.forTriangle(mesh,7)));

            Cad3dDwgHandleCrosscheck.Builder wrong=new Cad3dDwgHandleCrosscheck.Builder();
            wrong.addPage(0,new long[]{
                0xA1,Cad3dDwgHandleCrosscheck.TYPE_OTHER,
                0xB2,Cad3dDwgHandleCrosscheck.TYPE_LINE
            },2,false);
            Cad3dDwgHandleCrosscheck mismatched=wrong.finish();
            expect(!mismatched.matchesDirectModel(idx.forVertex(mesh,0)));
            expect(!mismatched.matchesDirectModel(idx.forTriangle(mesh,0)));

            Cad3dDwgHandleCrosscheck.Builder partial=new Cad3dDwgHandleCrosscheck.Builder();
            partial.addPage(0,new long[]{0xA1,1},1,true);
            try {partial.finish();throw new AssertionError("Incomplete set accepted");}
            catch(IOException good){}
            try {partial.addPage(2,new long[]{0xB2,2},3,false);
                 throw new AssertionError("Gap accepted");}
            catch(IOException good){}
            try {partial.addPage(1,new long[]{0xB2,2},2,false);
                 throw new AssertionError("Failed builder restarted");}
            catch(IOException good){}
        }finally {Files.deleteIfExists(file.toPath());}

        reject(new long[]{},0,0,false);
        reject(new long[]{0xA1},0,1,false);
        reject(new long[]{0,1},0,1,false);
        reject(new long[]{0xA1,99},0,1,false);
        reject(new long[]{0xA1,1,0xA1,2},0,2,false);
        reject(new long[]{0xA1,1},1,2,false);
        reject(new long[]{0xA1,1},0,3,false);
        System.out.println("Native original DWG model identity cross-check contract PASS");
    }
}
