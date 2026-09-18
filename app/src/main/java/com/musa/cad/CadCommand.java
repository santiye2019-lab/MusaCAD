package com.musa.cad;

import java.util.Locale;

/**
 * AutoCAD-style English command aliases for the mobile command line.
 * Supported commands execute in MusaCAD; familiar but not-yet-implemented
 * commands are recognized so experienced CAD users get an explicit status.
 */
public final class CadCommand {
    public enum Action {
        NONE, LINE, POLYLINE, CIRCLE, RECTANGLE, TEXT, SELECT,
        PAN, MOVE, COPY, ROTATE, ERASE, SCALE, MIRROR, OFFSET, ARRAY, EXPLODE, OSNAP, REGEN, TRIM, EXTEND, LAYER, PROPERTIES,
        DISTANCE, AREA, ZOOM, ZOOM_EXTENTS, UNDO, SAVE, HELP, UNSUPPORTED
    }

    public static Action parse(String raw){
        String s=normalize(raw);if(s.isEmpty())return Action.NONE;

        if(eq(s,"L","LINE"))return Action.LINE;
        if(eq(s,"PL","PLINE","POLYLINE"))return Action.POLYLINE;
        if(eq(s,"C","CIRCLE"))return Action.CIRCLE;
        if(eq(s,"REC","RECTANG","RECTANGLE"))return Action.RECTANGLE;
        if(eq(s,"DT","TEXT","T","MTEXT"))return Action.TEXT;

        // Do not use S for SELECT: classic AutoCAD users expect S = STRETCH.
        if(eq(s,"SEL","SELECT"))return Action.SELECT;
        if(eq(s,"P","PAN"))return Action.PAN;
        if(eq(s,"M","MOVE"))return Action.MOVE;
        if(eq(s,"CO","CP","COPY"))return Action.COPY;
        if(eq(s,"RO","ROTATE"))return Action.ROTATE;
        if(eq(s,"E","ERASE","DELETE"))return Action.ERASE;
        if(eq(s,"SC","SCALE"))return Action.SCALE;
        if(eq(s,"MI","MIRROR"))return Action.MIRROR;
        if(eq(s,"O","OFFSET"))return Action.OFFSET;
        if(eq(s,"AR","ARRAY"))return Action.ARRAY;
        if(eq(s,"X","EXPLODE"))return Action.EXPLODE;
        if(eq(s,"OS","OSNAP"))return Action.OSNAP;
        if(eq(s,"RE","REGEN"))return Action.REGEN;
        if(eq(s,"TR","TRIM"))return Action.TRIM;
        if(eq(s,"EX","EXTEND"))return Action.EXTEND;

        if(eq(s,"LA","LAYER"))return Action.LAYER;
        if(eq(s,"PR","PROPERTIES","PROP"))return Action.PROPERTIES;
        if(eq(s,"DI","DIST","DISTANCE"))return Action.DISTANCE;
        if(eq(s,"AA","AREA"))return Action.AREA;

        if(eq(s,"ZE","Z E","Z EXTENTS","ZOOM E","ZOOM EXTENTS"))return Action.ZOOM_EXTENTS;
        if(eq(s,"Z","ZOOM"))return Action.ZOOM;

        if(eq(s,"U","UNDO"))return Action.UNDO;
        if(eq(s,"QS","QSAVE","SAVE"))return Action.SAVE;
        if(eq(s,"?","HELP"))return Action.HELP;

        if(classicUnsupported(s)!=null)return Action.UNSUPPORTED;
        return Action.NONE;
    }

    public static String canonical(String raw){
        String s=normalize(raw);
        String unsupported=classicUnsupported(s);
        if(unsupported!=null)return unsupported;
        Action a=parse(raw);
        switch(a){
            case LINE:return "LINE";case POLYLINE:return "PLINE";case CIRCLE:return "CIRCLE";
            case RECTANGLE:return "RECTANG";case TEXT:return "TEXT/MTEXT";case SELECT:return "SELECT";
            case PAN:return "PAN";case MOVE:return "MOVE";case COPY:return "COPY";case ROTATE:return "ROTATE";
            case ERASE:return "ERASE";case SCALE:return "SCALE";case MIRROR:return "MIRROR";case OFFSET:return "OFFSET";
            case ARRAY:return "ARRAY";case EXPLODE:return "EXPLODE";case OSNAP:return "OSNAP";case REGEN:return "REGEN";
            case TRIM:return "TRIM";case EXTEND:return "EXTEND";
            case LAYER:return "LAYER";case PROPERTIES:return "PROPERTIES";
            case DISTANCE:return "DIST";case AREA:return "AREA";case ZOOM:return "ZOOM";
            case ZOOM_EXTENTS:return "ZOOM EXTENTS";case UNDO:return "UNDO";case SAVE:return "QSAVE";
            case HELP:return "HELP";default:return s;
        }
    }

    private static String classicUnsupported(String s){
        if(eq(s,"A","ARC"))return "ARC";
        if(eq(s,"B","BLOCK"))return "BLOCK";
        if(eq(s,"BR","BREAK"))return "BREAK";
        if(eq(s,"CHA","CHAMFER"))return "CHAMFER";
        if(eq(s,"D","DIMSTYLE"))return "DIMSTYLE";
        if(eq(s,"DAL","DIMALIGNED"))return "DIMALIGNED";
        if(eq(s,"DLI","DIMLINEAR"))return "DIMLINEAR";
        if(eq(s,"EL","ELLIPSE"))return "ELLIPSE";
        if(eq(s,"F","FILLET"))return "FILLET";
        if(eq(s,"H","HATCH"))return "HATCH";
        if(eq(s,"I","INSERT"))return "INSERT";
        if(eq(s,"J","JOIN"))return "JOIN";
        if(eq(s,"LI","LIST"))return "LIST";
        if(eq(s,"MA","MATCHPROP"))return "MATCHPROP";
        if(eq(s,"PE","PEDIT"))return "PEDIT";
        if(eq(s,"PO","POINT"))return "POINT";
        if(eq(s,"S","STRETCH"))return "STRETCH";
        if(eq(s,"XL","XLINE"))return "XLINE";
        if(eq(s,"REDO"))return "REDO";
        return null;
    }

    private static String normalize(String raw){
        if(raw==null)return "";
        String s=raw.trim().toUpperCase(Locale.ROOT)
            .replace('İ','I').replaceAll("\\s+"," ");
        return s;
    }

    private static boolean eq(String s,String... values){
        for(String value:values)if(s.equals(value))return true;
        return false;
    }
    private CadCommand(){}
}
