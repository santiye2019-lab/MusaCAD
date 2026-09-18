package com.musa.cad;

import java.util.Locale;

/** Small AutoCAD-like command alias parser used by the mobile command line. */
public final class CadCommand {
    public enum Action { NONE, LINE, POLYLINE, CIRCLE, RECTANGLE, TEXT, SELECT, MOVE, COPY, ERASE, LAYER, PROPERTIES, ZOOM_EXTENTS, UNDO, SAVE, HELP }
    public static Action parse(String raw){
        if(raw==null)return Action.NONE;String s=raw.trim().toUpperCase(Locale.ROOT).replace("İ","I");
        if(s.isEmpty())return Action.NONE;
        if(eq(s,"L","LINE","CIZGI","ÇIZGI","ÇİZGİ"))return Action.LINE;
        if(eq(s,"PL","PLINE","POLYLINE","COKLU","ÇOKLU"))return Action.POLYLINE;
        if(eq(s,"C","CIRCLE","DAIRE","DAİRE"))return Action.CIRCLE;
        if(eq(s,"REC","RECTANG","RECTANGLE","DORTGEN","DÖRTGEN"))return Action.RECTANGLE;
        if(eq(s,"T","TEXT","MTEXT","YAZI"))return Action.TEXT;
        if(eq(s,"S","SELECT","SEC","SEÇ"))return Action.SELECT;
        if(eq(s,"M","MOVE","TASI","TAŞI"))return Action.MOVE;
        if(eq(s,"CO","CP","COPY","KOPYA"))return Action.COPY;
        if(eq(s,"E","ERASE","DEL","DELETE","SIL","SİL"))return Action.ERASE;
        if(eq(s,"LA","LAYER","KATMAN"))return Action.LAYER;
        if(eq(s,"PR","PROPERTIES","PROP","OZELLIK","ÖZELLİK","ÖZELLIK"))return Action.PROPERTIES;
        if(eq(s,"Z","ZE","EXTENTS","FIT","SIGDIR","SIĞDIR"))return Action.ZOOM_EXTENTS;
        if(eq(s,"U","UNDO","GERI","GERİ"))return Action.UNDO;
        if(eq(s,"SAVE","KAYDET","DXFSAVE"))return Action.SAVE;
        if(eq(s,"?","HELP","YARDIM"))return Action.HELP;
        return Action.NONE;
    }
    private static boolean eq(String s,String...v){for(String x:v)if(s.equals(x))return true;return false;}
    private CadCommand(){}
}
