import com.musa.cad.CadCommand;

/**
 * Source-bound smoke for 2026 indexed command names, NOT AutoCAD behavior parity.
 * Parsing a command into an action proves only recognition. Actual drawing,
 * 2026 options, saved DWG and full UI behavior need independent tests.
 */
public final class AutoCAD2026CommandRouterSmokeTest {
    private static int inspected=0;
    private static void action(String name,CadCommand.Action expected){
        CadCommand.Action actual=CadCommand.parse(name);
        if(actual!=expected)throw new AssertionError(name+" parsed "+actual+" expected "+expected);
        inspected++;
    }
    public static void main(String[] args){
        // Directly indexed AutoCAD 2026 names that MusaCAD's router maps today.
        action("3DORBIT",CadCommand.Action.VIEW_3D); // A 3D view shortcut, not CAD orbit parity.
        action("ARC",CadCommand.Action.ARC);
        action("AREA",CadCommand.Action.AREA);
        action("ARRAY",CadCommand.Action.ARRAY);
        action("BLOCK",CadCommand.Action.BLOCK);
        action("BREAK",CadCommand.Action.BREAK);
        action("CHAMFER",CadCommand.Action.CHAMFER);
        action("CIRCLE",CadCommand.Action.CIRCLE);
        action("HATCH",CadCommand.Action.HATCH);
        action("MTEXT",CadCommand.Action.TEXT);
        action("PAN",CadCommand.Action.PAN);
        action("-HATCH",CadCommand.Action.NONE); // Command-line variant != regular form.
        action("-INSERT",CadCommand.Action.NONE);
        action("-PDFIMPORT",CadCommand.Action.NONE);
        action("3DPRINT",CadCommand.Action.NONE);
        action("ACISIN",CadCommand.Action.NONE);
        action("ACISOUT",CadCommand.Action.NONE);
        action("ARX",CadCommand.Action.NONE);
        action("ACTRECORD",CadCommand.Action.NONE);
        action("AUTOCONSTRAIN",CadCommand.Action.NONE);
        action("BEDIT",CadCommand.Action.NONE);
        action("BDETECT",CadCommand.Action.NONE);
        action("BVSTATE",CadCommand.Action.NONE);
        action("BOX",CadCommand.Action.NONE);
        action("CENTERMARK",CadCommand.Action.NONE);
        if(inspected!=25)throw new AssertionError("Unexpected reference smoke count");
        System.out.println("AutoCAD2026CommandRouterSmokeTest OK: "+inspected+
                " names checked for parser recognition only; parity unverified");
    }
}
