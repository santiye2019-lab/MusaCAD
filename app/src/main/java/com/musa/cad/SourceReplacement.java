package com.musa.cad;

/** Replacement geometry for one source DXF entity, retaining source layer and resolved color. */
public final class SourceReplacement {
    public final int sourceId;
    public final SourceRange range;
    public final String layer;
    public final int color;
    public final CadEdit edit;

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,CadEdit edit){
        if(sourceId<0||range==null||edit==null)throw new IllegalArgumentException("source replacement");
        this.sourceId=sourceId;this.range=range;this.layer=layer==null||layer.trim().isEmpty()?"0":layer;this.color=color;this.edit=edit.copy();
    }
    public SourceReplacement copy(){return new SourceReplacement(sourceId,range,layer,color,edit);}
}
