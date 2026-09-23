import com.musa.cad.DxfWipeout;
import java.util.*;

public final class DxfWipeoutTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static List<String> p(String...v){return Arrays.asList(v);}
    public static void main(String[]args){
        DxfWipeout.Result r=DxfWipeout.parse(p(
            "10","100","20","200","11","10","21","0","12","0","22","20","13","2","23","1",
            "71","2","91","4","14","-0.5","24","-0.5","14","1.5","24","-0.5","14","1.5","24","0.5","14","-0.5","24","0.5"
        ),0,36);
        require(r.boundary.size()==4,"clip boundary");
        require(Math.abs(r.boundary.get(0).x-100)<1e-9&&Math.abs(r.boundary.get(0).y-200)<1e-9,"origin");
        require(Math.abs(r.boundary.get(1).x-120)<1e-9&&Math.abs(r.boundary.get(2).y-220)<1e-9,"uv mapping");

        DxfWipeout.Result fallback=DxfWipeout.parse(p(
            "10","5","20","7","11","3","21","0","12","0","22","4","13","2","23","3"
        ),0,16);
        require(fallback.boundary.size()==4,"default rectangle");
        require(Math.abs(fallback.boundary.get(2).x-11)<1e-9&&Math.abs(fallback.boundary.get(2).y-19)<1e-9,"default extent");
        System.out.println("DxfWipeoutTest OK");
    }
}
