package com.musa.cad;

/** Source DXF line-index range belonging to one directly editable entity. */
public final class SourceRange {
    public final int sourceId;
    public final int startLine;
    public final int endLineExclusive;

    public SourceRange(int sourceId,int startLine,int endLineExclusive){
        if(sourceId<0||startLine<0||endLineExclusive<=startLine)throw new IllegalArgumentException("source range");
        this.sourceId=sourceId;this.startLine=startLine;this.endLineExclusive=endLineExclusive;
    }

    public boolean containsLine(int lineIndex){return lineIndex>=startLine&&lineIndex<endLineExclusive;}
}
