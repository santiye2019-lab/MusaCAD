import com.musa.cad.Cad3dVisibility;

public final class Cad3dVisibilityTest {
    private static void check(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    public static void main(String[]args){
        check(Cad3dVisibility.edgeVisible(-20,50,120,50,100,100,0),
            "line crossing viewport must remain visible");
        check(!Cad3dVisibility.edgeVisible(-50,-40,-20,-10,100,100,0),
            "fully outside line must be culled");
        check(Cad3dVisibility.triangleVisible(-50,50,150,50,50,150,100,100,0),
            "large crossing triangle must not be culled");
        check(!Cad3dVisibility.triangleVisible(120,130,140,120,150,170,100,100,0),
            "fully outside triangle must be culled");
        check(Cad3dVisibility.triangleVisible(105,50,108,45,108,55,100,100,8),
            "padded edge should be retained");
        check(!Cad3dVisibility.triangleVisible(Float.NaN,0,1,1,2,2,100,100,0),
            "NaN triangle must not enter painter sort");
        check(!Cad3dVisibility.edgeVisible(Float.POSITIVE_INFINITY,0,1,1,100,100,0),
            "nonfinite segment must not be drawn");
        System.out.println("3D visibility culling and crossing-polygon safety passed");
    }
}
