package com.musa.cad;

/** Source DXF line-index range belonging to one directly editable entity. */
public final class SourceRange {
    public final int sourceId;
    public final int startLine;
    public final int endLineExclusive;
    /** Provenance marker for removal of a BREAK2 original even if no children survive. */
    public final boolean breakSource;

    public SourceRange(int sourceId,int startLine,int endLineExclusive){
        this(sourceId,startLine,endLineExclusive,false);
    }
    public SourceRange(int sourceId,int startLine,int endLineExclusive,boolean breakSource){
        if(sourceId<0||startLine<0||endLineExclusive<=startLine)throw new IllegalArgumentException("source range");
        this.sourceId=sourceId;this.startLine=startLine;this.endLineExclusive=endLineExclusive;
        this.breakSource=breakSource;
    }

    public boolean containsLine(int lineIndex){return lineIndex>=startLine&&lineIndex<endLineExclusive;}
}
