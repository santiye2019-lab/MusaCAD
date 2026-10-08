import com.musa.cad.MusaAiVisualSweepPlan;
import java.util.*;

public final class MusaAiVisualSweepPlanTest {
    private static void check(boolean yes,String msg){
        if(!yes)throw new AssertionError(msg);
    }
    public static void main(String[]args){
        List<MusaAiVisualSweepPlan.Tile> tiles=MusaAiVisualSweepPlan.tiles();
        check(tiles.size()==9,"nine high-resolution tiles");
        check(MusaAiVisualSweepPlan.BATCH_COUNT==3,"three bounded provider batches");
        boolean[][] covered=new boolean[36][36];
        Set<String> names=new HashSet<>();
        int total=0;
        for(int batch=0;batch<MusaAiVisualSweepPlan.BATCH_COUNT;batch++){
            int first=MusaAiVisualSweepPlan.firstTile(batch);
            int end=MusaAiVisualSweepPlan.lastExclusive(batch);
            check(end>first&&end-first<=4,"batch bounded");
            total+=end-first;
            for(int k=first;k<end;k++){
                MusaAiVisualSweepPlan.Tile t=tiles.get(k);
                check(names.add(t.label()),"unique tile identity");
                check(t.index==k,"stable zero-based mapping");
                check(t.left>=0&&t.top>=0&&t.right<=1&&t.bottom<=1,
                    "normalized tile bounds");
                check(t.left<t.right&&t.top<t.bottom,"positive extent");
                for(int y=0;y<36;y++)for(int x=0;x<36;x++){
                    double cx=(x+0.5)/36d,cy=(y+0.5)/36d;
                    if(cx>=t.left&&cx<=t.right&&cy>=t.top&&cy<=t.bottom)
                        covered[y][x]=true;
                }
            }
        }
        check(total==9&&names.size()==9,"no duplicate or skipped detailed tiles");
        for(int y=0;y<36;y++)for(int x=0;x<36;x++)
            check(covered[y][x],"full normalized sheet coverage at "+x+","+y);
        check(tiles.get(0).label().equals("sheet-tile-1"),"tile one");
        check(tiles.get(8).label().equals("sheet-tile-9"),"tile nine");
        boolean rejected=false;
        try{MusaAiVisualSweepPlan.firstTile(3);}catch(IllegalArgumentException expected){rejected=true;}
        check(rejected,"reject invalid batch");
        System.out.println("MusaAiVisualSweepPlanTest OK");
    }
}
