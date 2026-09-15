import com.musa.cad.DxfBlocks;

public class DxfExpansionLimitTest {
    public static void main(String[] args){
        if(DxfBlocks.MAX_EXPANSION_VISITS<600000)throw new AssertionError("real drawing expansion ceiling regressed");
        if(DxfBlocks.MAX_EXPANSION_VISITS>1000000)throw new AssertionError("expansion safety ceiling is too loose");
        System.out.println("DXF real-drawing expansion safety ceiling passed");
    }
}
