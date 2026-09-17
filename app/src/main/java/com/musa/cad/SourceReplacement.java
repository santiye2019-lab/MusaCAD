package com.musa.cad;

/** Replacement geometry for one source DXF entity, retaining source display style. */
public final class SourceReplacement {
    public final int sourceId;
    public final SourceRange range;
    public final String layer;
    public final int color;
    public final String lineType;
    public final double lineTypeScale;
    public final int lineWeight;
    public final CadEdit edit;

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,CadEdit edit){
        this(sourceId,range,layer,color,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT,edit);
    }

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,String lineType,double lineTypeScale,int lineWeight,CadEdit edit){
        if(sourceId<0||range==null||edit==null)throw new IllegalArgumentException("source replacement");
        this.sourceId=sourceId;this.range=range;this.layer=layer==null||layer.trim().isEmpty()?"0":layer;this.color=color;
        this.lineType=DxfLineStyle.normalizeName(lineType);this.lineTypeScale=Double.isFinite(lineTypeScale)&&lineTypeScale>0d?lineTypeScale:1d;
        this.lineWeight=DxfLineStyle.normalizeWeight(lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT);this.edit=edit.copy();
    }
    public SourceReplacement copy(){return new SourceReplacement(sourceId,range,layer,color,lineType,lineTypeScale,lineWeight,edit);}
}
