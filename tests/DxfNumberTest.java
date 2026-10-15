import com.musa.cad.DxfNumber;
import java.io.IOException;
import java.util.Locale;

public final class DxfNumberTest {
    private static void yes(boolean condition,String note){
        if(!condition)throw new AssertionError(note);
    }
    public static void main(String[]args)throws Exception{
        Locale before=Locale.getDefault();
        try{
            Locale.setDefault(Locale.forLanguageTag("tr-TR"));
            for(double value:new double[]{0,1,-8.123456789012345,123456789.12345679,0.0000000001245,
                                          1000000000000.333,Double.MIN_NORMAL}){
                String encoded=DxfNumber.format(value);
                yes(!encoded.contains(",")&&!encoded.contains("E"),"DXF format must be plain decimal");
                yes(Double.parseDouble(encoded)==value,"real group lost numeric precision: "+value);
            }
            yes("0".equals(DxfNumber.format(-0.0)),"zero canonical output");
            boolean bad=false;
            try{DxfNumber.format(Double.NaN);}catch(IOException ok){bad=true;}
            yes(bad,"NaN coordinate must be blocked");
            bad=false;
            try{DxfNumber.format(Double.POSITIVE_INFINITY);}catch(IOException ok){bad=true;}
            yes(bad,"Infinite coordinate must be blocked");
        }finally{Locale.setDefault(before);}
        System.out.println("DXF full-precision finite decimal output and Turkish locale passed");
    }
}
