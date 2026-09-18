import com.musa.cad.CadCommand;

public final class CadCommandTest {
    private static void expect(String raw,CadCommand.Action expected){
        CadCommand.Action actual=CadCommand.parse(raw);
        if(actual!=expected)throw new AssertionError(raw+": "+actual+" != "+expected);
    }
    public static void main(String[] args){
        expect("L",CadCommand.Action.LINE);expect("PL",CadCommand.Action.POLYLINE);expect("C",CadCommand.Action.CIRCLE);
        expect("A",CadCommand.Action.ARC);expect("EL",CadCommand.Action.ELLIPSE);expect("PO",CadCommand.Action.POINT);expect("XL",CadCommand.Action.XLINE);
        expect("REC",CadCommand.Action.RECTANGLE);expect("T",CadCommand.Action.TEXT);expect("M",CadCommand.Action.MOVE);expect("CO",CadCommand.Action.COPY);
        expect("RO",CadCommand.Action.ROTATE);expect("E",CadCommand.Action.ERASE);expect("SC",CadCommand.Action.SCALE);expect("MI",CadCommand.Action.MIRROR);
        expect("O",CadCommand.Action.OFFSET);expect("AR",CadCommand.Action.ARRAY);expect("X",CadCommand.Action.EXPLODE);expect("OS",CadCommand.Action.OSNAP);
        expect("TR",CadCommand.Action.TRIM);expect("EX",CadCommand.Action.EXTEND);expect("F",CadCommand.Action.FILLET);expect("CHA",CadCommand.Action.CHAMFER);
        expect("BR",CadCommand.Action.BREAK);expect("PE",CadCommand.Action.PEDIT);expect("LI",CadCommand.Action.LIST);expect("MA",CadCommand.Action.MATCHPROP);
        expect("J",CadCommand.Action.JOIN);expect("H",CadCommand.Action.HATCH);expect("S",CadCommand.Action.STRETCH);expect("B",CadCommand.Action.BLOCK);
        expect("I",CadCommand.Action.INSERT);expect("D",CadCommand.Action.DIMSTYLE);expect("DLI",CadCommand.Action.DIMLINEAR);expect("DAL",CadCommand.Action.DIMALIGNED);
        expect("LA",CadCommand.Action.LAYER);expect("PR",CadCommand.Action.PROPERTIES);expect("DI",CadCommand.Action.DISTANCE);expect("AA",CadCommand.Action.AREA);
        expect("ZE",CadCommand.Action.ZOOM_EXTENTS);expect("U",CadCommand.Action.UNDO);expect("REDO",CadCommand.Action.REDO);expect("QS",CadCommand.Action.SAVE);
        expect("?",CadCommand.Action.HELP);
        if(CadCommand.parse("NOT_A_COMMAND")==CadCommand.Action.UNSUPPORTED)throw new AssertionError("unknown command should not be unsupported");
        System.out.println("CadCommandTest OK");
    }
}
