package com.musa.cad;

import java.util.Locale;
import java.util.Map;

/** Pure DXF linetype/lineweight inheritance helpers. */
public final class DxfStyle {
    public static final String BYLAYER="BYLAYER";
    public static final String BYBLOCK="BYBLOCK";
    public static final String CONTINUOUS="CONTINUOUS";
    public static final int LW_BYLAYER=-1;
    public static final int LW_BYBLOCK=-2;
    public static final int LW_DEFAULT=-3;

    public static String normalizeLineType(String raw){
        if(raw==null||raw.trim().isEmpty())return BYLAYER;
        return raw.trim().toUpperCase(Locale.ROOT);
    }

    /** Resolve BYBLOCK immediately; retain BYLAYER until the effective layer table is known. */
    public static String resolveLineType(String raw,String byBlock){
        String value=normalizeLineType(raw);
        if(BYBLOCK.equals(value)){
            String inherited=normalizeLineType(byBlock);
            return BYBLOCK.equals(inherited)?BYLAYER:inherited;
        }
        return value;
    }

    public static int resolveLineWeight(int raw,int byBlock){
        if(raw==LW_BYBLOCK)return byBlock==LW_BYBLOCK?LW_BYLAYER:byBlock;
        if(raw==LW_BYLAYER||raw==LW_DEFAULT||raw>=0)return raw;
        return LW_DEFAULT;
    }

    public static String effectiveLineType(String semantic,String layer,Map<String,String> layerTypes){
        String value=normalizeLineType(semantic);
        if(BYLAYER.equals(value)||BYBLOCK.equals(value)){
            String layerType=layerTypes==null?null:layerTypes.get(DxfColor.key(layer));
            value=normalizeLineType(layerType==null?CONTINUOUS:layerType);
        }
        if(BYLAYER.equals(value)||BYBLOCK.equals(value))return CONTINUOUS;
        return value;
    }

    public static int effectiveLineWeight(int semantic,String layer,Map<String,Integer> layerWeights){
        int value=semantic;
        if(value==LW_BYLAYER||value==LW_BYBLOCK){
            Integer layerValue=layerWeights==null?null:layerWeights.get(DxfColor.key(layer));
            value=layerValue==null?LW_DEFAULT:layerValue;
        }
        if(value<0)return LW_DEFAULT;
        return value;
    }

    /** Device-pixel width. DXF lineweights are hundredths of a millimetre. */
    public static float strokeWidthPx(int lineWeight){
        if(lineWeight<0)return 1.5f;
        if(lineWeight==0)return 1.0f;
        float px=(lineWeight/100f)*3.2f;
        return Math.max(1.0f,Math.min(8.0f,px));
    }

    public static double saneScale(double raw){
        return Double.isFinite(raw)&&Math.abs(raw)>1e-9?Math.abs(raw):1d;
    }

    private DxfStyle(){}
}
