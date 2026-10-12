import com.musa.cad.CadFrameDrawTimes;

public final class CadFrameDrawTimesTest {
    private static void check(boolean p,String msg){if(!p)throw new AssertionError(msg);}
    public static void main(String[]args){
        CadFrameDrawTimes t=new CadFrameDrawTimes();
        check(t.safeSummary().contains("Henüz"),"initially empty");
        for(int i=1;i<=100;i++)t.record(i*1_000_000L);
        String s=t.safeSummary();
        check(s.contains("100 kare"),"count");
        check(s.contains("p50=50.0 ms"),"median");
        check(s.contains("p95=95.0 ms"),"p95");
        check(s.contains("16.7ms üstü=84%"),"over budget");
        t.clear();check(t.safeSummary().contains("Henüz"),"reset on model switch");
        for(int i=0;i<500;i++)t.record(1_000_000L);
        check(t.safeSummary().contains("128 kare"),"ring memory remains bounded");
        System.out.println("CAD Canvas CPU draw timing p50/p95 rolling diagnostics passed");
    }
}
