import com.musa.cad.*;

public class SourceEditSessionTest {
    public static void main(String[] args){
        SourceEditSession s=new SourceEditSession();
        SourceRange r=new SourceRange(12,10,20);
        s.select(12,r,CadEdit.line(0,0,10,0),"BORU",0xFF00FF00,"DASHED",.5,50);
        if(!s.hasSelection())throw new AssertionError("selection");
        if(!s.moveSelectedTo(20,20))throw new AssertionError("move");
        CadEdit moved=s.currentSelected();near(moved.centerX(),20,"move center x");near(moved.centerY(),20,"move center y");
        if(!s.hiddenSourceIds().contains(12)||s.replacements().size()!=1||s.removals().size()!=1)throw new AssertionError("replacement state");
        SourceReplacement styled=s.replacementRecords().get(0);
        if(!"BORU".equals(styled.layer)||styled.color!=0xFF00FF00||!"DASHED".equals(styled.lineType)||styled.lineWeight!=50||Math.abs(styled.lineTypeScale-.5)>1e-9)
            throw new AssertionError("source style lost");
        if(!s.updateSelectedStyle("YENI_KATMAN",0xFFFF0000,"CENTER",2.0,35))throw new AssertionError("style update");
        SourceReplacement updated=s.replacementRecords().get(0);
        if(!"YENI_KATMAN".equals(updated.layer)||updated.color!=0xFFFF0000||!"CENTER".equals(updated.lineType)||Math.abs(updated.lineTypeScale-2.0)>1e-9||updated.lineWeight!=35)throw new AssertionError("style update values");
        if(!s.undo())throw new AssertionError("undo style");
        SourceReplacement styleUndo=s.replacementRecords().get(0);
        if(!"BORU".equals(styleUndo.layer)||styleUndo.color!=0xFF00FF00||!"DASHED".equals(styleUndo.lineType))throw new AssertionError("style undo values");
        if(!s.redo())throw new AssertionError("redo style");
        SourceReplacement styleRedo=s.replacementRecords().get(0);
        if(!"YENI_KATMAN".equals(styleRedo.layer)||styleRedo.color!=0xFFFF0000||!"CENTER".equals(styleRedo.lineType))throw new AssertionError("style redo values");
        if(!s.rotateSelected(90))throw new AssertionError("rotate");
        CadEdit copy=s.copySelected(5,0);if(copy==null)throw new AssertionError("copy");near(copy.centerX(),25,"copy center x");
        if(s.findReplacement(20,20,2)<0)throw new AssertionError("replacement hit");
        if(!s.deleteSelected())throw new AssertionError("delete");
        if(s.hasSelection()||s.replacements().size()!=0||s.modifiedCount()!=1)throw new AssertionError("delete state");
        if(!s.undo())throw new AssertionError("undo delete");
        if(!s.hasSelection()||s.replacements().size()!=1)throw new AssertionError("undo restore");
        SourceReplacement restored=s.replacementRecords().get(0);
        if(!"YENI_KATMAN".equals(restored.layer)||!"CENTER".equals(restored.lineType)||restored.lineWeight!=35||restored.color!=0xFFFF0000)throw new AssertionError("undo delete preserves edited style");
        System.out.println("Source entity edit session/style cases passed");
    }
    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.01f)throw new AssertionError(name+": "+actual+" != "+expected);}
}
