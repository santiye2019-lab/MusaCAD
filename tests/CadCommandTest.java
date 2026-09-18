import com.musa.cad.CadCommand;

public class CadCommandTest {
    public static void main(String[] args){
        String[] commands={"L","PL","C","A","EL","PO","XL","REC","TEXT","SEL","P","M","CO","RO","E","SC","MI","O","AR","X","OS","RE","TR","EX","F","CHA","BR","PE","LI","MA","J","H","S","B","I","D","DLI","DAL","LA","PR","DI","AA","ZE","Z","U","REDO","QS","?"};
        for(String command:commands){
            CadCommand.Action action=CadCommand.parse(command);
            if(action==CadCommand.Action.NONE||action==CadCommand.Action.UNSUPPORTED)throw new AssertionError(command+" not implemented: "+action);
        }
        if(CadCommand.parse("nonsense")!=CadCommand.Action.NONE)throw new AssertionError("unknown command");
        if(CadCommand.parse("S")!=CadCommand.Action.STRETCH)throw new AssertionError("S must be STRETCH");
        if(CadCommand.parse("D")!=CadCommand.Action.DIMSTYLE)throw new AssertionError("D must be DIMSTYLE");
        System.out.println("All MusaCAD command aliases are routed to implemented actions");
    }
}
