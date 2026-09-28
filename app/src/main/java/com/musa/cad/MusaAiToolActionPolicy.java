package com.musa.cad;

import java.util.*;

/**
 * Security policy for CAD commands proposed by a remote AI.
 * Unknown commands are denied. Mutating/file commands always require explicit
 * user confirmation before MainActivity may execute them.
 */
public final class MusaAiToolActionPolicy {
    public enum Risk { READ_ONLY, EDIT, FILE }

    public static final class Decision {
        public final boolean allowed;
        public final boolean requiresConfirmation;
        public final Risk risk;
        public final String command;
        public final String reason;
        private Decision(boolean allowed,boolean confirm,Risk risk,String command,String reason){
            this.allowed=allowed;this.requiresConfirmation=confirm;this.risk=risk;
            this.command=command;this.reason=reason;
        }
    }

    private static final Set<String> READ_ONLY=set(
        "ZE","ZOOM","PAN","3D","2D","LA","PR","DI","AA","ANG","ID","ARCLEN","HELP","LIST"
    );
    private static final Set<String> EDIT=set(
        "SELECT","MOVE","COPY","ERASE","ROTATE","SCALE","MIRROR","OFFSET","ARRAY","EXPLODE",
        "TRIM","EXTEND","FILLET","CHAMFER","BREAK","PEDIT","MATCHPROP","JOIN","HATCH","STRETCH",
        "BLOCK","INSERT","DIVIDE","REVCLOUD","MLEADER","PLINE","XLINE","LINE","CIRCLE","ARC",
        "ELLIPSE","POINT","RECTANG","TEXT","DLI","DAL","DAN","DRA","DDI","UNDO","REDO"
    );
    private static final Set<String> FILE=set("SAVE","QSAVE");

    public static Decision evaluate(String raw){
        String command=raw==null?"":raw.trim().toUpperCase(Locale.ROOT);
        if(command.isEmpty())return denied(command,"Boş komut.");
        int space=command.indexOf(' ');
        String verb=space<0?command:command.substring(0,space);
        if(READ_ONLY.contains(verb)){
            return new Decision(true,false,Risk.READ_ONLY,command,"Salt-okunur CAD işlemi.");
        }
        if(EDIT.contains(verb)){
            return new Decision(true,true,Risk.EDIT,command,"Çizimi değiştirebilir; kullanıcı onayı gerekir.");
        }
        if(FILE.contains(verb)){
            return new Decision(true,true,Risk.FILE,command,"Dosya durumunu değiştirebilir; kullanıcı onayı gerekir.");
        }
        return denied(command,"Komut Gandalf beyaz listesinde değil.");
    }

    private static Decision denied(String command,String reason){
        return new Decision(false,true,Risk.EDIT,command,reason);
    }
    private static Set<String>set(String...v){
        return Collections.unmodifiableSet(new LinkedHashSet<>(Arrays.asList(v)));
    }
    private MusaAiToolActionPolicy(){}
}
