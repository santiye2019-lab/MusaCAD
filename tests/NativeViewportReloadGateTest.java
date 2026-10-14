import com.musa.cad.NativeViewportReloadGate;

public final class NativeViewportReloadGateTest {
    private static void must(boolean result,String message){
        if(!result)throw new AssertionError(message);
    }
    private static void rejects(Runnable action,String message){
        try{action.run();throw new AssertionError(message);}
        catch(IllegalArgumentException expected){}
    }
    public static void main(String[] ignored){
        NativeViewportReloadGate gate=new NativeViewportReloadGate();
        must(gate.shouldRequest(-10,-10,10,10,0),"first visible request");
        must(!gate.shouldRequest(-10,-10,10,10,0),"duplicate frame suppressed");
        must(gate.shouldRequest(-10,-10,10,10,1),
             "new page may evict visible geometry at same zoom");
        must(!gate.shouldRequest(-10,-10,10,10,1),
             "same progress does not queue a decode loop");
        must(gate.shouldRequest(-11,-10,10,10,1),"pan changes viewport");
        must(gate.shouldRequest(-11,-10,10,10,50000),
             "large streaming discovery still refreshes unchanged viewport");
        must(!gate.shouldRequest(-11,-10,10,10,50000),
             "prevent per-frame redundant requests after EOF");
        must(gate.shouldRequest(-0.0f,0.0f,1,1,50000),"new coordinates");
        must(!gate.shouldRequest(0.0f,-0.0f,1,1,50000),
             "signed zero is the same world location");
        rejects(()->gate.shouldRequest(0,0,1,1,49999),
                "page discovery rollback is forbidden");
        rejects(()->gate.shouldRequest(Float.NaN,0,1,1,50000),
                "NaN viewport denied");
        rejects(()->gate.shouldRequest(1,0,0,1,50000),
                "inverted viewport denied");
        rejects(()->gate.shouldRequest(0,0,1,1,-1),
                "negative page count denied");
        must(!gate.shouldRequest(0,0,1,1,50000),
             "invalid requests preserve the last accepted signed-zero key");
        must(gate.shouldRequest(0,0,2,2,50000),
             "new viewport still accepted after invalid requests");
        must(!gate.shouldRequest(0,0,2,2,50000),
             "identical valid request suppressed");
        System.out.println("Native viewport decode-generation reload gate PASS");
    }
}
