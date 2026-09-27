import com.musa.cad.DxfOleFrame;
import java.util.*;

public final class DxfOleFrameTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    public static void main(String[]args){
        DxfOleFrame.Result r=DxfOleFrame.parse(Arrays.asList(
            "70","2","10","30.13602472538446","20","-18.98882829402869",
            "11","35.27188116753285","21","-22.39344715050545","90","113280"
        ),0,10);
        require(r.valid(),"real OLE frame must be valid");
        require(Math.abs(r.x1-30.13602472538446)<1e-12,"x1");
        require(Math.abs(r.y2+22.39344715050545)<1e-12,"y2");
        require(!DxfOleFrame.parse(Arrays.asList("10","1","20","2"),0,4).valid(),"incomplete frame must be rejected");

        DxfOleFrame.Result excel=DxfOleFrame.parse(Arrays.asList(
            "10","0","20","0","11","100","21","40",
            "3","Excel.Sheet.12",
            "310","89504E470D0A1A0A0000000D49484452"
        ),0,12);
        require(excel.valid(),"embedded Excel OLE frame must be valid");
        require("EXCEL".equals(excel.objectType),"Excel OLE type must be detected");
        require(excel.hasPayload(),"OLE binary payload must be collected");
        byte[] raster=DxfOleFrame.rasterPreview(excel.payload);
        require(raster!=null&&raster.length>=8,"embedded PNG preview must be found");
        require((raster[0]&255)==0x89&&raster[1]==0x50,"PNG signature must be preserved");
        System.out.println("DXF OLE2FRAME boundary and preview cases passed");
    }
}
