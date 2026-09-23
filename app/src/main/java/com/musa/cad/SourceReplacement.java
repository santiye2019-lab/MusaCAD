package com.musa.cad;

/** Replacement geometry for one source DXF entity, retaining source display style. */
public final class SourceReplacement {
    public static final int COLOR_EXPLICIT=0,COLOR_BYLAYER=1,COLOR_BYBLOCK=2;
    public final int sourceId;
    public final SourceRange range;
    public final String layer;
    public final int color,colorMode;
    public final String lineType;
    public final double lineTypeScale;
    public final int lineWeight;
    public final CadEdit edit;

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,CadEdit edit){
        this(sourceId,range,layer,color,COLOR_EXPLICIT,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT,edit);
    }

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,String lineType,double lineTypeScale,int lineWeight,CadEdit edit){
        this(sourceId,range,layer,color,COLOR_EXPLICIT,lineType,lineTypeScale,lineWeight,edit);
    }
    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,int colorMode,String lineType,double lineTypeScale,int lineWeight,CadEdit edit){
        if(sourceId<0||range==null||edit==null)throw new IllegalArgumentException("source replacement");
        this.sourceId=sourceId;this.range=range;this.layer=layer==null||layer.trim().isEmpty()?"0":layer;this.color=color;
        this.colorMode=colorMode==COLOR_BYLAYER||colorMode==COLOR_BYBLOCK?colorMode:COLOR_EXPLICIT;
        this.lineType=DxfLineStyle.normalizeName(lineType);this.lineTypeScale=Double.isFinite(lineTypeScale)&&lineTypeScale>0d?lineTypeScale:1d;
        this.lineWeight=DxfLineStyle.normalizeWeight(lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT);this.edit=edit.copy();
    }
    public SourceReplacement copy(){return new SourceReplacement(sourceId,range,layer,color,colorMode,lineType,lineTypeScale,lineWeight,edit);}
}
