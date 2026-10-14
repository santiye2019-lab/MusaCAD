import com.musa.cad.NativeSceneRecordBounds;

public final class NativeSceneRecordBoundsTest {
    private static void yes(boolean b,String why){
        if(!b)throw new AssertionError(why);
    }
    public static void main(String[] args){
        yes(NativeSceneRecordBounds.hasVertexPairs(3,4,11),
                "four 2D vertices exactly fit");
        yes(!NativeSceneRecordBounds.hasVertexPairs(3,5,11),
                "fifth vertex exceeds source words");
        yes(!NativeSceneRecordBounds.hasVertexPairs(3,Integer.MAX_VALUE,11),
                "overflowing vertex count must be rejected");
        yes(!NativeSceneRecordBounds.hasVertexPairs(-1,1,11),
                "negative source index rejected");
        yes(!NativeSceneRecordBounds.hasVertexPairs(12,0,11),
                "out-of-range source index rejected");
        yes(NativeSceneRecordBounds.packedTextWords(0,0)==0,
                "empty text needs no float words");
        yes(NativeSceneRecordBounds.packedTextWords(1,1)==1,
                "single UTF8 byte occupies one word");
        yes(NativeSceneRecordBounds.packedTextWords(4,2)==2,
                "four UTF8 bytes occupy two words");
        yes(NativeSceneRecordBounds.packedTextWords(4,1)==-1,
                "truncated packed text rejected");
        yes(NativeSceneRecordBounds.packedTextWords(Integer.MAX_VALUE,
                Integer.MAX_VALUE)==-1,"huge text count rejected before allocation");
        yes(NativeSceneRecordBounds.packedTextWords(-1,1)==-1,
                "negative text bytes rejected");
        yes(NativeSceneRecordBounds.countMatches(123,123),
                "complete declared primitive sequence");
        yes(!NativeSceneRecordBounds.countMatches(123,124),
                "extra native records must be detected");
        yes(!NativeSceneRecordBounds.countMatches(123,122),
                "truncated native records must be detected");
        yes(!NativeSceneRecordBounds.countMatches(0,0),
                "empty native geometry is not an accepted drawing");
        System.out.println("Native DWG stream record bounds and completion safety PASS");
    }
}
