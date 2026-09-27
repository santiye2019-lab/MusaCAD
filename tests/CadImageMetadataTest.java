import com.musa.cad.CadImageMetadata;
import com.musa.cad.CadImageOverlay;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

public final class CadImageMetadataTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void near(float actual,float expected,String message){if(Math.abs(actual-expected)>1e-4f)throw new AssertionError(message+": "+actual+" != "+expected);}
    public static void main(String[]args)throws Exception{
        CadImageOverlay image=new CadImageOverlay("img_01.webp","Cephe Görseli.webp",12.5f,-4.25f,300f,180f,37.5f);
        String encoded=CadImageMetadata.encode(image);
        require(encoded.startsWith(CadImageMetadata.PREFIX),"metadata prefix");
        CadImageOverlay decoded=CadImageMetadata.decode(encoded);
        require(decoded!=null&&decoded.key.equals(image.key)&&decoded.name.equals(image.name),"round trip identity");
        near(decoded.centerX,image.centerX,"round trip X");near(decoded.centerY,image.centerY,"round trip Y");
        near(decoded.width,image.width,"round trip width");near(decoded.height,image.height,"round trip height");near(decoded.rotationDegrees,image.rotationDegrees,"round trip rotation");
        require(CadImageMetadata.decode("OTHER|bad")==null,"foreign comment ignored");

        File file=File.createTempFile("musacad-images-", ".dxf");
        try{
            try(BufferedWriter out=Files.newBufferedWriter(file.toPath(),StandardCharsets.ISO_8859_1)){
                out.write("0\nSECTION\n2\nENTITIES\n");
                CadImageMetadata.writeComments(out,Collections.singletonList(image));
                out.write("0\nENDSEC\n0\nEOF\n");
            }
            List<CadImageOverlay> read=CadImageMetadata.read(file);
            require(read.size()==1,"one metadata record");
            require(read.get(0).name.equals("Cephe Görseli.webp"),"UTF-8 escaped name restored");
            near(read.get(0).rotationDegrees,37.5f,"file rotation");
        }finally{file.delete();}
        System.out.println("DXF image placement metadata round trip passed");
    }
}
