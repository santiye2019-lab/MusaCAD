import com.musa.cad.DxfViewport;
import java.util.*;

public class DxfViewportTest {
    private static List<String> tags(Object... values){ArrayList<String>a=new ArrayList<>();for(Object v:values)a.add(String.valueOf(v));return a;}
    private static void near(double actual,double expected,String name){if(Math.abs(actual-expected)>1e-6)throw new AssertionError(name+": "+actual+" != "+expected);}
    public static void main(String[] args){
        List<String>a=tags(10,100,20,50,40,200,41,100,68,2,69,3,12,25,22,10,45,500,51,0,16,0,26,0,36,1,17,1000,27,2000,90,0);
        DxfViewport.View v=DxfViewport.parse(a,0,a.size());if(!v.active2d())throw new AssertionError("active viewport");near(v.scale(),.2,"scale");near(v.left(),0,"left");near(v.right(),200,"right");near(v.bottom(),0,"bottom");near(v.top(),100,"top");near(v.modelCenterX(),1025,"model center x");near(v.modelCenterY(),2010,"model center y");
        List<String>twisted=tags(10,0,20,0,40,100,41,50,68,1,69,2,12,10,22,0,45,50,51,90,16,0,26,0,36,1,17,5,27,7);DxfViewport.View t=DxfViewport.parse(twisted,0,twisted.size());near(t.modelCenterX(),5,"twist center x");near(t.modelCenterY(),17,"twist center y");
        List<String>paperTags=tags(40,10,41,10,68,1,69,1,45,10,36,1);DxfViewport.View paper=DxfViewport.parse(paperTags,0,paperTags.size());if(paper.active2d())throw new AssertionError("paper viewport id 1 must not show model");
        List<String>offTags=tags(40,10,41,10,68,1,69,2,45,10,36,1,90,DxfViewport.FLAG_OFF);DxfViewport.View off=DxfViewport.parse(offTags,0,offTags.size());if(off.active2d())throw new AssertionError("off viewport");
        List<String>perspectiveTags=tags(40,10,41,10,68,1,69,2,45,10,36,1,90,DxfViewport.FLAG_PERSPECTIVE);DxfViewport.View perspective=DxfViewport.parse(perspectiveTags,0,perspectiveTags.size());if(perspective.active2d())throw new AssertionError("perspective viewport");
        List<String>clippedTags=tags(40,10,41,10,68,1,69,2,45,10,36,1,90,DxfViewport.FLAG_NON_RECTANGULAR);DxfViewport.View clipped=DxfViewport.parse(clippedTags,0,clippedTags.size());if(!clipped.active2d()||clipped.rectangular())throw new AssertionError("nonrect flag");
        System.out.println("DXF viewport geometry cases passed");
    }
}
