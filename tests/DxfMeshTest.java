import com.musa.cad.DxfMesh;

public class DxfMeshTest {
    private static void eq(int a,int b,String m){if(a!=b)throw new AssertionError(m+": "+a+" != "+b);}
    private static boolean has(double[] s,double x1,double y1,double x2,double y2){
        for(int i=0;i+3<s.length;i+=4){if(match(s[i],s[i+1],s[i+2],s[i+3],x1,y1,x2,y2)||match(s[i],s[i+1],s[i+2],s[i+3],x2,y2,x1,y1))return true;}return false;
    }
    private static boolean match(double a,double b,double c,double d,double e,double f,double g,double h){return Math.abs(a-e)<1e-9&&Math.abs(b-f)<1e-9&&Math.abs(c-g)<1e-9&&Math.abs(d-h)<1e-9;}
    public static void main(String[] args){
        double[] v={0,0,10,0,10,5,0,5};
        double[] square=DxfMesh.wireframe(v,new int[]{4,0,1,2,3},null);
        eq(square.length,16,"four unique square edges");
        if(!has(square,0,0,10,0)||!has(square,10,0,10,5)||!has(square,10,5,0,5)||!has(square,0,5,0,0))throw new AssertionError("square edges");
        double[] triangles=DxfMesh.wireframe(v,new int[]{3,0,1,2,3,0,2,3},null);
        eq(triangles.length,20,"shared diagonal deduped");
        double[] explicit=DxfMesh.wireframe(v,null,new int[]{0,1,1,2,2,3,3,0});eq(explicit.length,16,"explicit edges");
        double[] mixed=DxfMesh.wireframe(v,new int[]{3,0,1,2},new int[]{2,3,3,0});eq(mixed.length,20,"faces plus explicit edges");
        double[] bad=DxfMesh.wireframe(v,new int[]{4,0,1,9,3},new int[]{-1,2,1,1});eq(bad.length,12,"invalid references ignored");
        eq(DxfMesh.wireframe(new double[]{0,0},new int[]{2,0,1},null).length,0,"too few vertices");
        System.out.println("modern MESH wireframe cases passed");
    }
}
