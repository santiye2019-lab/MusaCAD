import com.musa.cad.CadFrameTimeStats;

public final class CadFrameTimeStatsTest {
    private static void yes(boolean condition,String label){
        if(!condition)throw new AssertionError(label);
    }
    private static void eq(double actual,double expected,String label){
        if(Math.abs(actual-expected)>1e-9)
            throw new AssertionError(label+" "+actual+" != "+expected);
    }
    public static void main(String[] args){
        CadFrameTimeStats metric=new CadFrameTimeStats();
        CadFrameTimeStats.Snapshot initial=metric.snapshot();
        yes(initial.moving.samples==0&&initial.settled.samples==0,
            "empty snapshot must be explicit");
        for(int i=1;i<=20;i++)metric.sample(i*1_000_000L,true);
        for(int i=1;i<=5;i++)metric.sample(i*2_000_000L,false);
        CadFrameTimeStats.Snapshot sample=metric.snapshot();
        yes(sample.moving.samples==20&&sample.settled.samples==5,
            "gesture and settled groups must stay separate");
        eq(sample.moving.p50Millis,10,"p50 nearest-rank gesture");
        eq(sample.moving.p95Millis,19,"p95 nearest-rank gesture");
        eq(sample.moving.maxMillis,20,"gesture max");
        yes(sample.moving.overBudget==4,"jank over 16.667 ms is counted");
        eq(sample.settled.p95Millis,10,"settled quantiles");
        try{
            metric.sample(-1,false);
            throw new AssertionError("negative duration accepted");
        }catch(IllegalArgumentException expected){}
        for(int i=0;i<300;i++)metric.sample(2_000_000L,true);
        CadFrameTimeStats.Snapshot rolled=metric.snapshot();
        yes(rolled.moving.samples==256,"memory must remain bounded at 256");
        eq(rolled.moving.p95Millis,2,"old long gesture frames must expire");
        yes(rolled.moving.overBudget==0,"old jank count must expire");
        yes(rolled.settled.samples==5,"gesture rollover cannot erase settled frames");
        System.out.println("CadView opt-in draw CPU quantile sampler PASS");
    }
}
