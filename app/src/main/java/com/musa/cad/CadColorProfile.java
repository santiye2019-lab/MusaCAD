package com.musa.cad;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * A bounded, color-only diagnostic for the Native DWG first frame and the
 * authoritative DXF renderer. No drawing coordinates, text, names or object
 * identifiers are stored. The two pipelines do not have identical geometry
 * counts, so this is a diagnostic, NOT an object-by-object fidelity proof.
 */
public final class CadColorProfile {
    public static final int MAX_SAMPLES=8192;
    private final HashMap<Integer,Integer> counts=new HashMap<>();
    private int sampled,dominantColor=0xFFFFFFFF,dominantCount,blueCount;

    public void add(int argb){
        if(sampled>=MAX_SAMPLES)return;
        int color=0xFF000000|(argb&0x00FFFFFF);
        int count=counts.containsKey(color)?counts.get(color)+1:1;
        counts.put(color,count);
        if(count>dominantCount){dominantColor=color;dominantCount=count;}
        if(color==0xFF0000FF)blueCount++;
        sampled++;
    }

    public int sampled(){return sampled;}
    public int unique(){return counts.size();}
    public int dominantColor(){return dominantColor;}
    public double dominantFraction(){return sampled==0?0d:dominantCount/(double)sampled;}
    public double blueFraction(){return sampled==0?0d:blueCount/(double)sampled;}

    /**
     * Only reports a strong, suspicious shift; legitimate drawings may be
     * predominantly blue. Samples represent primitives rather than pixels.
     */
    public boolean likelyFirstPaintMismatch(CadColorProfile authoritative){
        if(authoritative==null||sampled<48||authoritative.sampled<48)return false;
        return dominantFraction()>=.85d &&
            authoritative.dominantFraction()<=.55d &&
            dominantColor()!=authoritative.dominantColor();
    }

    public String safeSummary(){
        return String.format(Locale.ROOT,"sample=%d unique=%d dominant=#%06X %.1f%% ACI5blue=%.1f%%",
            sampled,unique(),dominantColor&0xFFFFFF,
            dominantFraction()*100d,blueFraction()*100d);
    }
    public CadColorProfile(){}
}
