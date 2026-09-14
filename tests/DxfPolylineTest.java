import com.musa.cad.DxfPolyline;
import java.util.*;
public class DxfPolylineTest {
    private static DxfPolyline poly(double bulge,boolean closed){
        List<String> tags=Arrays.asList("70",closed?"1":"0","10","0","20","0","42",Double.toString(bulge),"10","2","20","0","42","1");
        return DxfPolyline.parse(tags,0,tags.size());
    }
    private static void near(double a,double b,double tolerance){if(Math.abs(a-b)>tolerance)throw new AssertionError(a+" != "+b);}
    private static void arc(double bulge){
        DxfPolyline p=poly(bulge,false);List<float[]> curve=p.curve(0);
        double x0=0,y0=0,cy=(1/bulge-bulge)/2,r=Math.hypot(1,cy);
        for(float[] c:curve){
            for(int step=0;step<=10;step++){
                double t=step/10d,u=1-t;
                double x=u*u*u*x0+3*u*u*t*c[0]+3*u*t*t*c[2]+t*t*t*c[4];
                double y=u*u*u*y0+3*u*u*t*c[1]+3*u*t*t*c[3]+t*t*t*c[5];
                near(Math.hypot(x-1,y-cy),r,Math.max(2e-6,r*5e-6));
            }
            x0=c[4];y0=c[5];
        }
        near(x0,2,1e-6);near(y0,0,1e-6);
        if(Math.abs(bulge)>=1)near(curve.get(curve.size()/2-1)[5],-bulge,1e-5);
    }
    public static void main(String[] args)throws Exception{
        if(args.length>0){
            int[] counts={0,0};
            com.musa.cad.DxfBlocks.expand(new java.io.File(args[0]),java.nio.charset.StandardCharsets.UTF_8,p->{
                if(!p.record.type.equals("LWPOLYLINE"))return;
                DxfPolyline poly=DxfPolyline.parse(p.record.tags,p.record.from,p.record.to);counts[0]++;
                for(int i=0;i<poly.segmentCount();i++)if(!poly.curve(i).isEmpty())counts[1]++;
            });
            System.out.println("Polylines: "+counts[0]+", curved segments: "+counts[1]);return;
        }
        java.io.File sample=java.io.File.createTempFile("musacad-bulge-",".dxf");
        try{
            java.nio.file.Files.writeString(sample.toPath(),"0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n70\n0\n10\n0\n20\n0\n42\n1\n10\n2\n20\n0\n0\nENDSEC\n0\nEOF\n");
            int[] found={0};com.musa.cad.DxfBlocks.expand(sample,java.nio.charset.StandardCharsets.UTF_8,p->{
                DxfPolyline poly=DxfPolyline.parse(p.record.tags,p.record.from,p.record.to);
                if(poly.curve(0).size()!=4)throw new AssertionError("Stream discarded bulge");found[0]++;
            });
            if(found[0]!=1)throw new AssertionError("Fixture entity count");
        }finally{sample.delete();}

        for(double b:new double[]{1,-1,2,-2,Math.tan(Math.PI/8),-Math.tan(Math.PI/8),1e-10,1e5})arc(b);
        if(!poly(0,false).curve(0).isEmpty())throw new AssertionError("Straight segment");
        if(poly(0,false).segmentCount()!=1||poly(0,true).segmentCount()!=2||poly(0,true).curve(1).isEmpty())throw new AssertionError("Closing bulge");
        List<String> missing=Arrays.asList("10","0");
        try{DxfPolyline.parse(missing,0,missing.size());throw new AssertionError("Missing Y");}catch(IllegalArgumentException expected){}
        System.out.println("8 arc shapes, straight/closing segments and malformed vertex passed");
    }
}
