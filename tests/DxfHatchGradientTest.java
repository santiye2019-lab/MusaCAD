import com.musa.cad.DxfHatchGradient;
import java.util.*;

public class DxfHatchGradientTest {
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void close(double a,double b,String m){if(Math.abs(a-b)>1e-9)throw new AssertionError(m+": "+a+" != "+b);}
    private static List<String> tags(Object... values){ArrayList<String> r=new ArrayList<>();for(Object v:values)r.add(String.valueOf(v));return r;}
    public static void main(String[] args){
        DxfHatchGradient.Data two=DxfHatchGradient.parse(tags(
            450,1,451,0,460,Math.PI/6,461,.3,452,0,462,0,453,2,
            463,0,421,16711680,463,1,421,255,470,"LINEAR"),0,28);
        yes(two.enabled,"gradient enabled");yes(two.linear(),"linear type");yes(!two.oneColor,"two color");
        if(two.color1!=0xff0000||two.color2!=0x0000ff)throw new AssertionError("true colors");
        close(two.rotation,Math.PI/6,"rotation");close(two.centered,.3,"centered");

        DxfHatchGradient.Data one=DxfHatchGradient.parse(tags(
            450,1,452,1,462,.5,463,0,421,0x6496c8,463,1,421,0,470,"linear"),0,18);
        yes(one.enabled&&one.oneColor&&one.linear(),"one color mode");
        if(one.color1!=0x6496c8||one.color2!=0x808080)throw new AssertionError("one color tint");

        DxfHatchGradient.Data solid=DxfHatchGradient.parse(tags(450,0,452,0,470,"LINEAR"),0,6);
        yes(!solid.enabled,"450=0 must stay solid");
        DxfHatchGradient.Data radial=DxfHatchGradient.parse(tags(450,1,452,0,463,0,421,1,463,1,421,2,470,"SPHERICAL"),0,18);
        yes(radial.enabled&&!radial.linear(),"unsupported type remains identifiable");

        double[] a=DxfHatchGradient.axis(0,0,10,4,0);close(a[0],0,"axis left");close(a[1],2,"axis y0");close(a[2],10,"axis right");close(a[3],2,"axis y1");
        double[] b=DxfHatchGradient.axis(0,0,10,4,Math.PI/2);close(b[0],5,"vertical x0");close(b[1],0,"vertical y0");close(b[2],5,"vertical x1");close(b[3],4,"vertical y1");
        yes(DxfHatchGradient.axis(Double.NaN,0,1,1,0).length==0,"invalid axis");
        System.out.println("HATCH gradient metadata and axis cases passed");
    }
}
