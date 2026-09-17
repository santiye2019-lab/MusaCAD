package com.musa.cad;

import java.util.*;

/** Parses one DXF LTYPE table record without depending on Android classes. */
public final class DxfLineTypeParser {
    public static DxfLineStyle.Pattern parse(String name,List<String> tags,int from,int to){
        ArrayList<Double> elements=new ArrayList<>();
        ArrayList<DxfLineStyle.ComplexElement> complex=new ArrayList<>();
        Element current=null;
        int end=Math.min(to,tags==null?0:tags.size());
        for(int i=Math.max(0,from);i+1<end;i+=2){
            int code=intOf(tags.get(i));String value=tags.get(i+1)==null?"":tags.get(i+1).trim();
            if(code==49){
                if(current!=null)finish(current,complex);
                double length=doubleOf(value,0d);elements.add(length);current=new Element(elements.size()-1);
            }else if(current!=null){
                switch(code){
                    case 74:current.flags=intOf(value);break;
                    case 75:current.shapeNumber=intOf(value);break;
                    case 9:current.text=value;break;
                    case 46:current.scale=doubleOf(value,1d);break;
                    case 50:current.rotation=doubleOf(value,0d);break;
                    case 44:current.xOffset=doubleOf(value,0d);break;
                    case 45:current.yOffset=doubleOf(value,0d);break;
                    default:break;
                }
            }
        }
        if(current!=null)finish(current,complex);
        double[] raw=new double[elements.size()];for(int i=0;i<raw.length;i++)raw[i]=elements.get(i);
        return new DxfLineStyle.Pattern(name,raw,!complex.isEmpty(),complex);
    }

    private static void finish(Element e,List<DxfLineStyle.ComplexElement> out){
        if(e.flags==0&&e.shapeNumber==0&&(e.text==null||e.text.isEmpty()))return;
        out.add(new DxfLineStyle.ComplexElement(e.index,e.flags,e.shapeNumber,e.text,e.scale,e.rotation,e.xOffset,e.yOffset));
    }

    private static final class Element{
        final int index;int flags,shapeNumber;String text="";double scale=1d,rotation,xOffset,yOffset;
        Element(int index){this.index=index;}
    }
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return 0;}}
    private static double doubleOf(String s,double fallback){try{double v=Double.parseDouble(s.trim());return Double.isFinite(v)?v:fallback;}catch(Exception e){return fallback;}}
    private DxfLineTypeParser(){}
}
