import com.musa.cad.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;
public class DxfUnitsTest {
    private static int read(String body)throws Exception{
        File f=File.createTempFile("musa-units-",".dxf");
        try{Files.writeString(f.toPath(),body);return DxfUnits.read(f,StandardCharsets.UTF_8);}finally{f.delete();}
    }
    public static void main(String[] args)throws Exception{
        if(args.length>0){int unit=DxfUnits.read(new File(args[0]),StandardCharsets.UTF_8);System.out.println("File unit: "+unit+" ("+DxfUnits.label(unit)+")");return;}
        for(int code:new int[]{1,2,3,4,5,6,7,10,14}){
            if(read("0\nSECTION\n2\nHEADER\n9\n$INSUNITS\n70\n"+code+"\n0\nENDSEC\n0\nEOF\n")!=code)throw new AssertionError("Header code");
        }
        if(read("0\nSECTION\n2\nHEADER\n9\n$MEASUREMENT\n70\n1\n0\nENDSEC\n0\nEOF\n")!=0)throw new AssertionError("Guessed unit");
        if(!Double.isNaN(DxfUnits.metersPerPixel(0,224))||!Double.isNaN(DxfUnits.metersPerPixel(99,224)))throw new AssertionError("Unknown scale");
        if(Math.abs(10000*DxfUnits.metersPerUnit(4)-10)>1e-10)throw new AssertionError("mm to m");
        if(Math.abs(2240*DxfUnits.metersPerPixel(6,224)-10)>1e-10)throw new AssertionError("Pixels to meters");
        File f=File.createTempFile("musa-layout-",".dxf");
        try{
            Files.writeString(f.toPath(),"0\nSECTION\n2\nENTITIES\n0\nLINE\n67\n1\n10\n100\n20\n100\n11\n200\n21\n100\n0\nLINE\n10\n0\n20\n0\n11\n10\n21\n0\n0\nENDSEC\n0\nEOF\n");
            int[] count={0};DxfBlocks.Result r=DxfBlocks.expand(f,StandardCharsets.UTF_8,p->count[0]++);
            if(count[0]!=1||r.skipped!=1)throw new AssertionError("Paperspace not excluded");
        }finally{f.delete();}
        System.out.println("Unit header, conversion, unknown-unit and paperspace tests passed");
    }
}
