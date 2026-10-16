package com.musa.cad;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Pure-Java screen/print layer metadata for source LINE replacements.
 *
 * BREAK children may have moved to DIFFERENT CAD layers. Source entity
 * visibility and its old resolved RGB are not the child's current style.
 * Uses the actual parsed DXF LAYER table, never guesses another layer color.
 */
public final class DxfReplacementLayerStyle {
    private final Map<String,Integer> colors;
    private final Map<String,String> lineTypes;
    private final Map<String,Integer> lineWeights;

    public DxfReplacementLayerStyle(Map<String,Integer> colors,
                                    Map<String,String> lineTypes,
                                    Map<String,Integer> lineWeights){
        this.colors=normalized(colors);
        this.lineTypes=normalized(lineTypes);
        this.lineWeights=normalized(lineWeights);
    }

    public static DxfReplacementLayerStyle empty(){
        return new DxfReplacementLayerStyle(Collections.emptyMap(),
                                            Collections.emptyMap(),
                                            Collections.emptyMap());
    }

    private static <T> Map<String,T> normalized(Map<String,T> source){
        LinkedHashMap<String,T> result=new LinkedHashMap<>();
        if(source!=null)for(Map.Entry<String,T> e:source.entrySet())
            if(e.getValue()!=null)result.put(DxfLayerState.key(e.getKey()),e.getValue());
        return Collections.unmodifiableMap(result);
    }

    /**
     * A replacement keeps its SOURCE layout, but must obey its NEW layer
     * visibility. An unknown/frozen/off layer is deliberately not painted.
     */
    public boolean visible(SourceReplacement replacement,String sourceLayout,
                           String activeLayout,Set<String>visibleLayerKeys){
        return replacement!=null&&sourceLayout!=null&&activeLayout!=null&&
               sourceLayout.equalsIgnoreCase(activeLayout)&&visibleLayerKeys!=null&&
               visibleLayerKeys.contains(DxfLayerState.key(replacement.layer));
    }

    /** Return current child color from its explicit or BYLAYER style. */
    public int argb(SourceReplacement r){
        if(r==null)throw new IllegalArgumentException("source replacement");
        if(r.colorMode==SourceReplacement.COLOR_EXPLICIT)
            return DxfColor.trueColorArgb(r.color);
        if(r.colorMode==SourceReplacement.COLOR_BYBLOCK)
            return DxfColor.aciArgb(DxfColor.DEFAULT_ACI);
        return colors.getOrDefault(DxfLayerState.key(r.layer),
                                   DxfColor.aciArgb(DxfColor.DEFAULT_ACI));
    }

    public String lineType(SourceReplacement r){
        if(r==null)throw new IllegalArgumentException("source replacement");
        String kind=DxfLineStyle.normalizeName(r.lineType);
        if(DxfLineStyle.BYLAYER.equals(kind))
            return DxfLineStyle.normalizeName(
                lineTypes.getOrDefault(DxfLayerState.key(r.layer),
                                       DxfLineStyle.CONTINUOUS));
        if(DxfLineStyle.BYBLOCK.equals(kind))return DxfLineStyle.CONTINUOUS;
        return kind;
    }

    public int lineWeight(SourceReplacement r){
        if(r==null)throw new IllegalArgumentException("source replacement");
        return DxfLineStyle.resolveLineweight(
            r.lineWeight,
            lineWeights.getOrDefault(DxfLayerState.key(r.layer),
                                     DxfLineStyle.DEFAULT_LINEWEIGHT),
            DxfLineStyle.DEFAULT_LINEWEIGHT,
            DxfLineStyle.DEFAULT_LINEWEIGHT);
    }
}
