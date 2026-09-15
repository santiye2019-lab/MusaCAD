package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Reads DXF LTYPE dash patterns, including embedded text/shape metadata. */
public final class DxfLineTypes {
    public static final int KIND_NONE=0,KIND_SHAPE=1,KIND_TEXT=2;

    public static final class Element {
        public final double length,scale,rotation,xOffset,yOffset;
        public final int flags,shapeNumber;
        public final String text,styleHandle;
        Element(double length,int flags,int shapeNumber,String text,String styleHandle,double scale,double rotation,double xOffset,double yOffset){
            this.length=finite(length,0);this.flags=flags;this.shapeNumber=shapeNumber;this.text=clean(text);this.styleHandle=clean(styleHandle);
            this.scale=positive(scale,1);this.rotation=finite(rotation,0);this.xOffset=finite(xOffset,0);this.yOffset=finite(yOffset,0);
        }
        public int kind(){
            if(!text.isEmpty()||(flags&KIND_TEXT)!=0)return KIND_TEXT;
            if((flags&KIND_SHAPE)!=0||shapeNumber!=0)return KIND_SHAPE;
            return KIND_NONE;
        }
    }

    public static final class Pattern {
        public final String name;
        /** Kept for the existing dash renderer and compatibility with earlier tests. */
        public final double[] elements;
        public final Element[] sequence;
        Pattern(String name,Element[] sequence){
            this.name=name;this.sequence=sequence==null?new Element[0]:sequence;
            elements=new double[this.sequence.length];for(int i=0;i<elements.length;i++)elements[i]=this.sequence[i].length;
        }
        public boolean continuous(){return elements.length==0;}
        public boolean complex(){for(Element e:sequence)if(e.kind()!=KIND_NONE)return true;return false;}
    }

    public static final class Placement {
        public final int kind,flags,shapeNumber;
        public final String text,styleHandle;
        public final double distance,scalePixels,rotation,xOffsetPixels,yOffsetPixels;
        Placement(Element e,double distance,double factor){
            kind=e.kind();flags=e.flags;shapeNumber=e.shapeNumber;text=e.text;styleHandle=e.styleHandle;this.distance=distance;
            scalePixels=Math.abs(e.scale)*factor;rotation=e.rotation;xOffsetPixels=e.xOffset*factor;yOffsetPixels=e.yOffset*factor;
        }
    }

    public static final class Table {
        public final Map<String,Pattern> patterns=new HashMap<>();
        public double globalScale=1d;

        public void add(String name,List<Double> values){
            ArrayList<Element> sequence=new ArrayList<>();if(values!=null)for(Double value:values)sequence.add(new Element(value==null?0:value,0,0,"","",1,0,0,0));
            addSequence(name,sequence);
        }
        private void addSequence(String name,List<Element> sequence){
            String key=DxfStyle.normalizeLineType(name);patterns.put(key,new Pattern(key,sequence.toArray(new Element[0])));
        }
        public Pattern get(String name){return patterns.get(DxfStyle.normalizeLineType(name));}

        /** Adds one LTYPE record whose list starts at the record's first group code after group 0/LTYPE. */
        public void addRecord(List<String> record)throws IOException{
            if(record==null)return;String name=value(record,2,DxfStyle.CONTINUOUS);ArrayList<Element> sequence=new ArrayList<>();Builder current=null;
            try{
                for(int i=0;i+1<record.size();i+=2){
                    int c=code(record.get(i));String raw=record.get(i+1);
                    if(c==49){if(current!=null)sequence.add(current.build());current=new Builder(number(raw));continue;}
                    if(current==null)continue;
                    switch(c){
                        case 74:current.flags=(int)number(raw);break;
                        case 75:current.shapeNumber=(int)number(raw);break;
                        case 340:current.styleHandle=raw;break;
                        case 46:current.scale=number(raw);break;
                        case 50:current.rotation=number(raw);break;
                        case 44:current.xOffset=number(raw);break;
                        case 45:current.yOffset=number(raw);break;
                        case 9:current.text=raw;break;
                        default:break;
                    }
                }
            }catch(NumberFormatException e){throw new IOException("Geçersiz DXF LTYPE elemanı",e);}
            if(current!=null)sequence.add(current.build());addSequence(name,sequence);
        }
    }

    private static final class Builder {
        final double length;int flags,shapeNumber;String text="",styleHandle="";double scale=1,rotation,xOffset,yOffset;
        Builder(double length){this.length=length;}
        Element build(){return new Element(length,flags,shapeNumber,text,styleHandle,scale,rotation,xOffset,yOffset);}
    }

    public static Table parse(List<String> tags)throws IOException{
        Table table=new Table();String section="";
        try{
            for(int i=0;i+1<tags.size();i+=2){
                int c=code(tags.get(i));
                if(c==9&&"$LTSCALE".equalsIgnoreCase(tags.get(i+1).trim())){
                    for(int j=i+2;j+1<tags.size()&&j<i+12;j+=2){
                        int q=code(tags.get(j));
                        if(q==40){double v=number(tags.get(j+1));if(Double.isFinite(v)&&v>0)table.globalScale=v;break;}
                        if(q==9||q==0)break;
                    }
                }
            }
            for(int i=0;i<tags.size();){
                if(code(tags.get(i))!=0){i+=2;continue;}
                String type=tags.get(i+1).trim();int from=i+2;i=from;
                while(i<tags.size()&&code(tags.get(i))!=0)i+=2;
                if("SECTION".equals(type)){section=text(tags,from,i,2,"").trim();continue;}
                if("ENDSEC".equals(type)){section="";continue;}
                if(!"TABLES".equals(section)||!"LTYPE".equals(type))continue;
                table.addRecord(new ArrayList<>(tags.subList(from,i)));
            }
        }catch(NumberFormatException e){throw new IOException("Geçersiz DXF LTYPE tablosu",e);}
        if(table.get(DxfStyle.CONTINUOUS)==null)table.add(DxfStyle.CONTINUOUS,Collections.emptyList());return table;
    }

    public static float[] dashIntervals(Pattern pattern,double scale,double deviceScale){
        if(pattern==null||pattern.elements.length==0)return null;
        ArrayList<Float> out=new ArrayList<>();double factor=factor(scale,deviceScale);
        for(double raw:pattern.elements){
            double len=Math.abs(raw)*factor;if(raw==0)len=Math.max(.75,0.02*factor);
            out.add((float)Math.max(.5,Math.min(10000d,len)));
        }
        if(out.size()==1)out.add(out.get(0));
        if((out.size()&1)==1){int n=out.size();for(int i=0;i<n;i++)out.add(out.get(i));}
        float[] r=new float[out.size()];for(int i=0;i<r.length;i++)r[i]=out.get(i);return r;
    }

    /** Returns embedded text/shape placement points measured along one already-transformed segment in pixels. */
    public static List<Placement> decorations(Pattern pattern,double scale,double deviceScale,double segmentPixels){
        if(pattern==null||!pattern.complex()||!Double.isFinite(segmentPixels)||segmentPixels<=0)return Collections.emptyList();
        double factor=factor(scale,deviceScale),cycle=0;for(Element e:pattern.sequence)cycle+=Math.abs(e.length)*factor;
        if(!Double.isFinite(cycle)||cycle<1e-6)return Collections.emptyList();
        ArrayList<Placement> out=new ArrayList<>();double phase=0;
        for(Element e:pattern.sequence){
            if(e.kind()!=KIND_NONE){
                for(double d=phase;d<=segmentPixels+1e-6;d+=cycle){out.add(new Placement(e,d,factor));if(out.size()>=4096)return out;}
            }
            phase+=Math.abs(e.length)*factor;
        }
        return out;
    }

    private static double factor(double scale,double deviceScale){return Math.max(1e-6,DxfStyle.saneScale(scale)*Math.max(1e-6,deviceScale));}
    private static int code(String s){return Integer.parseInt(s.trim());}
    private static double number(String s){double v=Double.parseDouble(s.trim());if(!Double.isFinite(v))throw new NumberFormatException();return v;}
    private static String value(List<String> tags,int wanted,String fallback){for(int i=0;i+1<tags.size();i+=2)if(code(tags.get(i))==wanted)return tags.get(i+1);return fallback;}
    private static String text(List<String> tags,int from,int to,int wanted,String fallback){for(int i=from;i+1<to;i+=2)if(code(tags.get(i))==wanted)return tags.get(i+1);return fallback;}
    private static String clean(String s){return s==null?"":s.trim();}
    private static double finite(double v,double fallback){return Double.isFinite(v)?v:fallback;}
    private static double positive(double v,double fallback){return Double.isFinite(v)&&v>1e-12?v:fallback;}
    private DxfLineTypes(){}
}
