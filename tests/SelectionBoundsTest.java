import com.musa.cad.SelectionBounds;
public class SelectionBoundsTest {
    private static void check(SelectionBounds b,int l,int t,int w,int h){
        if(b==null||b.left!=l||b.top!=t||b.width!=w||b.height!=h)throw new AssertionError("Unexpected crop bounds");
    }
    private static void empty(SelectionBounds b){if(b!=null)throw new AssertionError("Expected invalid selection");}
    public static void main(String[] args){
        check(SelectionBounds.clip(10,20,80,90,100,100,8),10,20,70,70);
        check(SelectionBounds.clip(80,90,10,20,100,100,8),10,20,70,70);
        check(SelectionBounds.clip(-50,-20,120,130,100,100,8),0,0,100,100);
        check(SelectionBounds.clip(10.2f,20.4f,80.1f,90.1f,100,100,8),10,20,71,71);
        empty(SelectionBounds.clip(5,5,5,5,100,100,8));
        empty(SelectionBounds.clip(5,5,10,90,100,100,8));
        empty(SelectionBounds.clip(120,5,200,90,100,100,8));
        empty(SelectionBounds.clip(Float.NaN,5,80,90,100,100,8));
        empty(SelectionBounds.clip(0,0,80,90,0,0,8));
        System.out.println("9 selection cases passed");
    }
}
