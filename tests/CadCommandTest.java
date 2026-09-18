import com.musa.cad.CadCommand;

public class CadCommandTest {
    private static void expect(String s,CadCommand.Action a){
        CadCommand.Action got=CadCommand.parse(s);
        if(got!=a)throw new AssertionError(s+" -> "+got+" expected "+a);
    }
    private static void canonical(String s,String expected){
        String got=CadCommand.canonical(s);
        if(!expected.equals(got))throw new AssertionError(s+" canonical "+got+" expected "+expected);
    }
    public static void main(String[] args){
        expect("L",CadCommand.Action.LINE);expect("line",CadCommand.Action.LINE);
        expect("PL",CadCommand.Action.POLYLINE);expect("pline",CadCommand.Action.POLYLINE);
        expect("C",CadCommand.Action.CIRCLE);expect("REC",CadCommand.Action.RECTANGLE);
        expect("DT",CadCommand.Action.TEXT);expect("T",CadCommand.Action.TEXT);expect("MTEXT",CadCommand.Action.TEXT);
        expect("SEL",CadCommand.Action.SELECT);expect("SELECT",CadCommand.Action.SELECT);
        expect("P",CadCommand.Action.PAN);expect("M",CadCommand.Action.MOVE);
        expect("CO",CadCommand.Action.COPY);expect("CP",CadCommand.Action.COPY);
        expect("RO",CadCommand.Action.ROTATE);expect("E",CadCommand.Action.ERASE);
        expect("LA",CadCommand.Action.LAYER);expect("PR",CadCommand.Action.PROPERTIES);
        expect("COL",CadCommand.Action.COLOR);expect("COLOR",CadCommand.Action.COLOR);
        expect("LW",CadCommand.Action.LINEWEIGHT);expect("LWEIGHT",CadCommand.Action.LINEWEIGHT);
        expect("LT",CadCommand.Action.LINETYPE);expect("LTYPE",CadCommand.Action.LINETYPE);
        expect("DI",CadCommand.Action.DISTANCE);expect("AA",CadCommand.Action.AREA);
        expect("Z",CadCommand.Action.ZOOM);expect("Z E",CadCommand.Action.ZOOM_EXTENTS);
        expect("ZOOM EXTENTS",CadCommand.Action.ZOOM_EXTENTS);expect("ZE",CadCommand.Action.ZOOM_EXTENTS);
        expect("U",CadCommand.Action.UNDO);expect("QSAVE",CadCommand.Action.SAVE);

        // Critical classic aliases must never be repurposed.
        expect("S",CadCommand.Action.UNSUPPORTED);canonical("S","STRETCH");
        expect("TR",CadCommand.Action.UNSUPPORTED);canonical("TR","TRIM");
        expect("EX",CadCommand.Action.UNSUPPORTED);canonical("EX","EXTEND");
        expect("O",CadCommand.Action.UNSUPPORTED);canonical("O","OFFSET");
        expect("SC",CadCommand.Action.UNSUPPORTED);canonical("SC","SCALE");
        expect("MI",CadCommand.Action.UNSUPPORTED);canonical("MI","MIRROR");
        expect("F",CadCommand.Action.UNSUPPORTED);canonical("F","FILLET");
        expect("xyz",CadCommand.Action.NONE);
        System.out.println("Classic AutoCAD command alias compatibility cases passed");
    }
}
