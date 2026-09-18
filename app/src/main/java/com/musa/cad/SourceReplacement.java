package com.musa.cad;

/** Replacement geometry for one source DXF entity, retaining or overriding source display style. */
public final class SourceReplacement {
    public final int sourceId;
    public final SourceRange range;
    public final String layer;
    /** Resolved ARGB convenience value used by legacy callers. */
    public final int color;
    public final int colorMode,colorValue;
    public final String lineType;
    public final double lineTypeScale;
    /** Raw DXF lineweight: -1 BYLAYER, -2 BYBLOCK, -3 DEFAULT, or hundredths of mm. */
    public final int lineWeight;
    public final CadEdit edit;

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,CadEdit edit){
        this(sourceId,range,layer,CadEdit.COLOR_TRUECOLOR,color&0x00FFFFFF,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT,edit);
    }

    public SourceReplacement(int sourceId,SourceRange range,String layer,int color,String lineType,double lineTypeScale,int lineWeight,CadEdit edit){
        this(sourceId,range,layer,CadEdit.COLOR_TRUECOLOR,color&0x00FFFFFF,lineType,lineTypeScale,lineWeight,edit);
    }

    public SourceReplacement(int sourceId,SourceRange range,String layer,int colorMode,int colorValue,String lineType,double lineTypeScale,int lineWeight,CadEdit edit){
        if(sourceId<0||range==null||edit==null)throw new IllegalArgumentException("source replacement");
        this.sourceId=sourceId;this.range=range;this.layer=layer==null||layer.trim().isEmpty()?"0":layer.trim();
        this.colorMode=colorMode>=CadEdit.COLOR_BYLAYER&&colorMode<=CadEdit.COLOR_TRUECOLOR?colorMode:CadEdit.COLOR_TRUECOLOR;
        if(this.colorMode==CadEdit.COLOR_ACI)this.colorValue=Math.max(1,Math.min(255,colorValue));
        else if(this.colorMode==CadEdit.COLOR_TRUECOLOR)this.colorValue=colorValue&0x00FFFFFF;
        else this.colorValue=colorValue;
        this.color=resolvedConvenienceColor(this.colorMode,this.colorValue);
        this.lineType=DxfLineStyle.normalizeName(lineType);this.lineTypeScale=Double.isFinite(lineTypeScale)&&lineTypeScale>0d?lineTypeScale:1d;
        this.lineWeight=rawLineWeight(lineWeight);this.edit=edit.copy();
    }

    private static int resolvedConvenienceColor(int mode,int value){
        if(mode==CadEdit.COLOR_ACI)return DxfColor.aciArgb(value);
        if(mode==CadEdit.COLOR_TRUECOLOR)return 0xFF000000|(value&0x00FFFFFF);
        return mode==CadEdit.COLOR_BYBLOCK?0xFF00FFFF:0xFFFFFFFF;
    }
    private static int rawLineWeight(int value){
        if(value==DxfLineStyle.LW_BYLAYER||value==DxfLineStyle.LW_BYBLOCK||value==DxfLineStyle.LW_DEFAULT)return value;
        return value>=0&&value<=211?value:DxfLineStyle.DEFAULT_LINEWEIGHT;
    }
    public SourceReplacement copy(){return new SourceReplacement(sourceId,range,layer,colorMode,colorValue,lineType,lineTypeScale,lineWeight,edit);}
}
