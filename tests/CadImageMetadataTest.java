import com.musa.cad.CadImageMetadata;
import com.musa.cad.CadImagePlacement;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

public final class CadImageMetadataTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void near(float actual,float expected,String message){if(Math.abs(actual-expected)>1e-4f)throw new AssertionError(message+": "+actual+" != "+expected);}
    public static void main(String[]args)throws Exception{
        CadImagePlacement original=new CadImagePlacement(
            "content://com.android.providers.media.documents/document/image%3A123",
            "Cephe Görseli.webp",12.5f,-4.25f,300f,180f,37.5f);
        String encoded=CadImageMetadata.encode(original);
        require(encoded.startsWith(CadImageMetadata.PREFIX),"metadata prefix");
        CadImagePlacement decoded=CadImageMetadata.decode(encoded);
        require(decoded!=null,"decode");
        require(decoded.uri.equals(original.uri),"URI round trip");
        require(decoded.name.equals(original.name),"name round trip");
        near(decoded.centerX,original.centerX,"center X");
        near(decoded.centerY,original.centerY,"center Y");
        near(decoded.width,original.width,"width");
        near(decoded.height,original.height,"height");
        near(decoded.rotationDegrees,original.rotationDegrees,"rotation");
        require(CadImageMetadata.decode("SOME_OTHER_999_COMMENT")==null,"foreign 999 ignored");

        File dxf=File.createTempFile("musacad-image-meta-", ".dxf");
        try{
            try(BufferedWriter out=Files.newBufferedWriter(dxf.toPath(),StandardCharsets.ISO_8859_1)){
                out.write("0\nSECTION\n2\nENTITIES\n");
                CadImageMetadata.writeComments(out,Collections.singletonList(original));
                out.write("0\nENDSEC\n0\nEOF\n");
            }
            List<CadImagePlacement> loaded=CadImageMetadata.read(dxf);
            require(loaded.size()==1,"one placement restored");
            require(loaded.get(0).name.equals("Cephe Görseli.webp"),"UTF-8 escaped name restored");
            near(loaded.get(0).rotationDegrees,37.5f,"file rotation");
        }finally{dxf.delete();}
        System.out.println("Raster DXF placement metadata round trip passed");
    }
}
