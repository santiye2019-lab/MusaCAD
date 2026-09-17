package com.musa.cad;

import java.util.*;

/** Pure-Java DXF linetype/lineweight helpers shared by parser tests and Android rendering. */
public final class DxfLineStyle {
    public static final String CONTINUOUS="CONTINUOUS";
    public static final String BYLAYER="BYLAYER";
    public static final String BYBLOCK="BYBLOCK";

    public static final int LW_BYLAYER=-1;
    public static final int LW_BYBLOCK=-2;
    public static final int LW_DEFAULT=-3;
    public static final int DEFAULT_LINEWEIGHT=25; // 0.25 mm

    /** Metadata attached to a complex LTYPE dash element. External SHX glyph bytes are never embedded here. */
    public static final class ComplexElement {
        public final int elementIndex,flags,shapeNumber;
        public final String text;
        public final double scale,rotationDegrees,xOffset,yOffset;

        public ComplexElement(int elementIndex,int flags,int shapeNumber,String text,double scale,double rotationDegrees,double xOffset,double yOffset){
            this.elementIndex=Math.max(0,elementIndex);
            this.flags=flags;
            this.shapeNumber=shapeNumber;
            this.text=text=text==null?"":text;
            this.scale=safeFinitePositive(scale,1d);
            this.rotationDegrees=Double.isFinite(rotationDegrees)?rotationDegrees:0d;
            this.xOffset=Double.isFinite(xOffset)?xOffset:0d;
            this.yOffset=Double.isFinite(yOffset)?yOffset:0d;
        }

        public boolean hasText(){return !text.isEmpty();}
        public boolean hasShape(){return shapeNumber!=0&&!hasText();}
    }

    public static final class Pattern {
        public final String name;
        public final double[] elements;
        public final boolean complex;
        public final List<ComplexElement> complexElements;

        public Pattern(String name,double[] elements,boolean complex){this(name,elements,complex,Collections.emptyList());}
        public Pattern(String name,double[] elements,boolean complex,List<ComplexElement> complexElements){
            this.name=normalizeName(name);
            this.elements=elements==null?new double[0]:elements.clone();
            ArrayList<ComplexElement> copy=new ArrayList<>();
            if(complexElements!=null)for(ComplexElement item:complexElements)if(item!=null&&item.elementIndex<this.elements.length)copy.add(item);
            this.complexElements=Collections.unmodifiableList(copy);
            this.complex=complex||!copy.isEmpty();
        }

        public boolean continuous(){return elements.length==0||CONTINUOUS.equals(name);}
        public boolean hasRenderableComplexText(){for(ComplexElement item:complexElements)if(item.hasText())return true;return false;}
        public boolean requiresExternalShape(){for(ComplexElement item:complexElements)if(item.hasShape())return true;return false;}

        /** Unscaled absolute DXF pattern cycle length. */
        public double cycleLength(){double total=0d;for(double value:elements)if(Double.isFinite(value))total+=Math.abs(value);return total;}

        /** Distance from the beginning of a pattern cycle to the middle of an element. */
        public double elementCenterDistance(int index){
            if(index<0||index>=elements.length)return 0d;
            double total=0d;for(int i=0;i<index;i++)total+=Math.abs(elements[i]);
            return total+Math.abs(elements[index])*.5d;
        }

        /**
         * Converts signed DXF dash/gap elements into Android-compatible on/off intervals.
         * Positive = dash, negative = gap, zero = dot. Phase preserves an initial gap.
         */
        public Dash dash(double pixelsPerDrawingUnit,double globalScale,double entityScale,double blockScale){
            if(continuous())return null;
            double factor=Math.abs(pixelsPerDrawingUnit*safeScale(globalScale)*safeScale(entityScale)*safeScale(blockScale));
            if(!Double.isFinite(factor)||factor<=1e-9)return null;
            ArrayList<Float> intervals=new ArrayList<>();
            float phase=0f;
            boolean wantOn=true;
            for(int i=0;i<elements.length;i++){
                double raw=elements[i];
                boolean on=raw>=0d;
                float length=(float)(Math.abs(raw)*factor);
                if(Math.abs(raw)<1e-12)length=Math.max(1f,(float)(factor*.03d));
                length=Math.max(.5f,Math.min(10000f,length));
                if(intervals.isEmpty()&&!on){
                    intervals.add(.5f);phase=.5f;wantOn=false;
                }
                if(on!=wantOn){intervals.add(.5f);wantOn=!wantOn;}
                intervals.add(length);wantOn=!wantOn;
            }
            if(intervals.size()<2)return null;
            if((intervals.size()&1)!=0)intervals.add(intervals.get(intervals.size()-1));
            float[] values=new float[intervals.size()];for(int i=0;i<values.length;i++)values[i]=intervals.get(i);
            return new Dash(values,phase);
        }
    }

    public static final class Dash {
        public final float[] intervals;
        public final float phase;
        Dash(float[] intervals,float phase){this.intervals=intervals;this.phase=phase;}
    }

    public static String normalizeName(String name){
        String value=name==null?"":name.trim();
        return value.isEmpty()?CONTINUOUS:value.toUpperCase(Locale.ROOT);
    }

    public static String resolveLinetype(String raw,String layerType,String inheritedBlockType){
        String value=normalizeName(raw);
        if(BYLAYER.equals(value))return normalizeName(layerType);
        if(BYBLOCK.equals(value))return normalizeName(inheritedBlockType);
        return value;
    }

    public static int resolveLineweight(int raw,int layerWeight,int inheritedBlockWeight,int defaultWeight){
        int fallback=validDefault(defaultWeight);
        if(raw==LW_BYLAYER)return normalizeWeight(layerWeight,fallback);
        if(raw==LW_BYBLOCK)return normalizeWeight(inheritedBlockWeight,fallback);
        return normalizeWeight(raw,fallback);
    }

    public static int normalizeWeight(int value,int defaultWeight){
        int fallback=validDefault(defaultWeight);
        if(value==LW_DEFAULT||value==LW_BYLAYER||value==LW_BYBLOCK)return fallback;
        if(value<0||value>211)return fallback;
        return value;
    }

    /** Approximate AutoCAD on-screen lineweight display in logical pixels. */
    public static float screenStroke(int hundredthsMm){
        int value=normalizeWeight(hundredthsMm,DEFAULT_LINEWEIGHT);
        if(value==0)return 1f;
        return Math.max(1f,Math.min(8f,(value/100f)*6f));
    }

    /** Physical PDF/print stroke in points. */
    public static float printStrokePoints(int hundredthsMm){
        int value=normalizeWeight(hundredthsMm,DEFAULT_LINEWEIGHT);
        if(value==0)return .25f;
        return Math.max(.25f,(value/100f)*72f/25.4f);
    }

    private static int validDefault(int value){return value>=0&&value<=211?value:DEFAULT_LINEWEIGHT;}
    private static double safeScale(double value){return Double.isFinite(value)&&value>0d?value:1d;}
    private static double safeFinitePositive(double value,double fallback){return Double.isFinite(value)&&value>0d?value:fallback;}
    private DxfLineStyle(){}
}
