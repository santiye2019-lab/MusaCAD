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
        // BREAK and BREAK2 must be ONE source transaction, not two overlay
        // edits with three separate undo steps. Both halves serialize with
        // exactly the same styling inherited from the selected source LINE.
        SourceEditSession split=new SourceEditSession();
        SourceRange sr=new SourceRange(99,100,115);
        split.select(99,sr,CadEdit.line(0,0,20,0),
                "M-ATIKSU",0xFF21BA56,"DASHED",0.75d,35);
        if(split.splitSelected(CadEdit.line(0,0,7,0),CadEdit.line(13,0,20,0))!=true)
            throw new AssertionError("2D source split must commit");
        if(split.hasSelection())throw new AssertionError("split must not ambiguously select one child");
        if(split.modifiedCount()!=1||split.removals().size()!=1||
           split.replacements().size()!=2||split.replacementRecords().size()!=2)
            throw new AssertionError("two output LINEs, exactly one original DXF removal");
        java.util.List<SourceReplacement> fragments=split.replacementRecords();
        for(int k=0;k<2;k++){
            SourceReplacement fragment=fragments.get(k);
            if(fragment.sourceId!=99||fragment.range!=sr||
               !"M-ATIKSU".equals(fragment.layer)||
               fragment.color!=0xFF21BA56||!"DASHED".equals(fragment.lineType)||
               fragment.lineWeight!=35||Math.abs(fragment.lineTypeScale-.75)>1e-9)
                throw new AssertionError("split lost source DXF style on part "+k);
        }
        near(fragments.get(0).edit.xy[2],7,"source first half");
        near(fragments.get(1).edit.xy[0],13,"source second half");
        if(split.findReplacement(15,0,2)>=0)
            throw new AssertionError("unaddressed child selection must fail closed");
        SourceEditSession frozen=split.snapshotCopy();
        if(!split.undo()||split.modifiedCount()!=0||split.replacements().size()!=0||
           split.removals().size()!=0||!split.hasSelection())
            throw new AssertionError("one undo must restore source LINE and style");
        if(split.undo())throw new AssertionError("source split must be just one undo");
        if(!split.redo()||split.modifiedCount()!=1||split.replacements().size()!=2)
            throw new AssertionError("one redo must restore both split segments");
        if(frozen.replacementRecords().size()!=2||
           frozen.replacementRecords().get(1).edit.xy[0]!=13)
            throw new AssertionError("tab snapshot must preserve complete split");
        SourceEditSession fromSnapshot=new SourceEditSession();
        fromSnapshot.copyFrom(frozen);
        if(!fromSnapshot.undo()||fromSnapshot.modifiedCount()!=0)
            throw new AssertionError("restored drawing session must undo entire split");
        SourceEditSession reject=new SourceEditSession();
        reject.select(9,new SourceRange(9,0,4),CadEdit.line(0,0,10,0),
                      "ELEKTRIK",0xFFABCDEF);
        if(reject.splitSelected(CadEdit.line(0,0,0,0),CadEdit.line(5,0,10,0))||
           reject.modifiedCount()!=0||reject.replacements().size()!=0)
            throw new AssertionError("degenerate split must not mutate source state");
        if(!reject.splitSelected(CadEdit.line(0,0,3,0),CadEdit.line(7,0,10,0)))
            throw new AssertionError("valid second split");
        if(reject.splitSelected(CadEdit.line(0,0,2,0),CadEdit.line(8,0,10,0)))
            throw new AssertionError("nested split without editable child IDs is unsafe");

        // Source DXF ID zero is legal; its child ID must not collide with
        // the -1 sentinel for 'nothing selected'.
        SourceEditSession zero=new SourceEditSession();
        zero.select(0,new SourceRange(0,0,5),CadEdit.line(0,0,10,0));
        if(!zero.splitSelected(CadEdit.line(0,0,3,0),CadEdit.line(7,0,10,0)))
            throw new AssertionError("source ID 0 split");
        SourceEditSession.SplitPartHit zeroLeft=zero.findSplitPart(1,0,0.1f);
        if(zeroLeft==null||zeroLeft.logicalId==-1||
           !zero.selectSplitPart(zeroLeft.sourceId,zeroLeft.part)||
           zero.selectedLogicalId()!=zeroLeft.logicalId)
            throw new AssertionError("zero-based source must have a real child identity");

        // Independent stable logical IDs for two BREAK children, while
        // preserving the ONE parent source range for DXF undo/save.
        SourceEditSession children=new SourceEditSession();
        SourceRange childRange=new SourceRange(123,300,315);
        children.select(123,childRange,CadEdit.line(0,0,20,0),
                        "TESISAT",0xFF27C0B2,"DASHED",0.5d,25);
        if(!children.splitSelected(CadEdit.line(0,0,7,0),
                                   CadEdit.line(13,0,20,0)))
            throw new AssertionError("logical child split");
        SourceEditSession.SplitPartHit left=children.findSplitPart(3,0,1);
        SourceEditSession.SplitPartHit right=children.findSplitPart(17,0,1);
        if(left==null||right==null||left.sourceId!=123||right.sourceId!=123||
           left.part!=0||right.part!=1||left.logicalId==right.logicalId||
           left.logicalId>=0||right.logicalId>=0)
            throw new AssertionError("child IDs must be distinct and stable");
        if(!children.selectSplitPart(left.sourceId,left.part)||
           children.selectedLogicalId()!=left.logicalId||children.selectedSplitPart()!=0)
            throw new AssertionError("select left fragment by hit");
        if(!children.moveSelectedTo(3,4))
            throw new AssertionError("move selected left child");
        java.util.List<SourceReplacement> splitMoves=children.replacementRecords();
        if(splitMoves.size()!=2)throw new AssertionError("move must retain sibling");
        near(splitMoves.get(0).edit.xy[1],4,"left child independently moved");
        near(splitMoves.get(1).edit.xy[1],0,"right child unchanged");
        for(SourceReplacement piece:splitMoves)
            if(!"TESISAT".equals(piece.layer)||piece.color!=0xFF27C0B2||
               !"DASHED".equals(piece.lineType)||piece.lineWeight!=25||
               Math.abs(piece.lineTypeScale-0.5d)>1e-9)
                throw new AssertionError("independently edited child lost source style");
        if(children.updateSelectedStyle("YANLIS",null,null,null,null)||
           children.updateSelectedColorMode(SourceReplacement.COLOR_BYBLOCK))
            throw new AssertionError("shared source style must not silently change a single child");
        if(children.copySelected(3,0)!=null||children.offsetSelected(1)!=null)
            throw new AssertionError("fragment copy/offset must wait for exact source styling");
        if(!children.undo()||children.replacementRecords().size()!=2)
            throw new AssertionError("undo child move");
        near(children.replacementRecords().get(0).edit.xy[1],0,"undo left move");
        if(!children.redo())throw new AssertionError("redo child move");
        near(children.replacementRecords().get(0).edit.xy[1],4,"redo left move");
        if(!children.selectSplitPart(right.sourceId,right.part))
            throw new AssertionError("select independent right child");
        if(children.selectedLogicalId()!=right.logicalId)
            throw new AssertionError("stable right logical ID after edits");
        if(!children.deleteSelected()||children.replacementRecords().size()!=1||
           children.removals().size()!=1||children.modifiedCount()!=1)
            throw new AssertionError("delete only right child, preserve parent removal");
        near(children.replacementRecords().get(0).edit.xy[1],4,"left child survives right delete");
        if(children.findSplitPart(17,0,1)!=null)
            throw new AssertionError("deleted right child cannot be selected");
        if(!children.undo()||children.replacementRecords().size()!=2)
            throw new AssertionError("undo must restore only deleted child");
        if(!children.selectSplitPart(left.sourceId,left.part)||!children.deleteSelected()||
           !children.selectSplitPart(right.sourceId,right.part)||!children.deleteSelected())
            throw new AssertionError("both children can be deleted independently");
        if(children.replacementRecords().size()!=0||
           children.removals().size()!=1||
           !children.hiddenSourceIds().contains(123))
            throw new AssertionError("when both children deleted parent DXF must remain removed");
        if(!children.undo()||children.replacementRecords().size()!=1||
           !children.redo()||children.replacementRecords().size()!=0)
            throw new AssertionError("deletion undo/redo must not restore unwanted sibling");
        SourceEditSession snapChildren=children.snapshotCopy();
        if(snapChildren.replacementRecords().size()!=0||
           snapChildren.removals().size()!=1)
            throw new AssertionError("split child delete state must persist across tabs");

        System.out.println("Source entity edit session/style cases passed");
    }
    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.01f)throw new AssertionError(name+": "+actual+" != "+expected);}
}
