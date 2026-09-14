import com.musa.cad.DxfTextLayout;

public class DxfTextLayoutTest {
    private static void near(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        near(DxfTextLayout.textHorizontal(0),0);near(DxfTextLayout.textHorizontal(1),.5);near(DxfTextLayout.textHorizontal(2),1);near(DxfTextLayout.textHorizontal(4),.5);
        if(DxfTextLayout.textVertical(3)!=3||DxfTextLayout.textVertical(99)!=0)throw new AssertionError();
        near(DxfTextLayout.mtextHorizontal(1),0);near(DxfTextLayout.mtextHorizontal(2),.5);near(DxfTextLayout.mtextHorizontal(3),1);
        if(DxfTextLayout.mtextVertical(1)!=0||DxfTextLayout.mtextVertical(5)!=1||DxfTextLayout.mtextVertical(9)!=2)throw new AssertionError();
        if(!DxfTextLayout.usesAlignmentPoint(1,0)||!DxfTextLayout.usesAlignmentPoint(0,2)||DxfTextLayout.usesAlignmentPoint(0,0))throw new AssertionError();
        if(!DxfTextLayout.isAlignedOrFit(3)||!DxfTextLayout.isAlignedOrFit(5)||DxfTextLayout.isAlignedOrFit(2))throw new AssertionError();
        System.out.println("DXF text alignment cases passed");
    }
}
