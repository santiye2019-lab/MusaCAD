import com.musa.cad.CadCommand;
public class CadCommandTest {
    private static void expect(String s,CadCommand.Action a){if(CadCommand.parse(s)!=a)throw new AssertionError(s+" -> "+CadCommand.parse(s));}
    public static void main(String[] args){
        expect("l",CadCommand.Action.LINE);expect("PL",CadCommand.Action.POLYLINE);expect("daire",CadCommand.Action.CIRCLE);
        expect("rec",CadCommand.Action.RECTANGLE);expect("text",CadCommand.Action.TEXT);expect("m",CadCommand.Action.MOVE);
        expect("co",CadCommand.Action.COPY);expect("erase",CadCommand.Action.ERASE);expect("la",CadCommand.Action.LAYER);
        expect("pr",CadCommand.Action.PROPERTIES);expect("ze",CadCommand.Action.ZOOM_EXTENTS);expect("u",CadCommand.Action.UNDO);
        expect("save",CadCommand.Action.SAVE);expect("?",CadCommand.Action.HELP);expect("xyz",CadCommand.Action.NONE);
        System.out.println("CAD command aliases passed");
    }
}
