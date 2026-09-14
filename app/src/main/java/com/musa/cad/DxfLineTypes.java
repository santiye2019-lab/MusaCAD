package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Reads simple DXF LTYPE dash patterns and converts them to device intervals. */
public final class DxfLineTypes {
    public static final class Pattern {
        public final String name; public final double[] elements;
        Pattern(String name,double[] elements){this.name=name;this.elements=elements;}
        public boolean continuous(){return elements.length==0;}
    }
    public static final class Table {
        public final Map<String,Pattern> patterns=new HashMap<>();
        public double globalScale=1d;
        public void add(String name,List<Double> values){
            String key=DxfStyle.normalizeLineType(name);
            double[] e=new double[values.size()];for(int i=0;i<e.length;i++)e[i]=values.get(i);
            patterns.put(key,new Pattern(key,e));
        }
        public Pattern get(String name){return patterns.get(DxfStyle.normalizeLineType(name));}
    }

    public static Table parse(List<String> tags)throws IOException{
        Table table=new Table();String section="";
        try{
            for(int i=0;i+1<tags.size();i+=2){
                int code=Integer.parseInt(tags.get(i).trim());
                if(code==9&&"$LTSCALE".equalsIgnoreCase(tags.get(i+1).trim())){
                    for(int j=i+2;j+1<tags.size()&&j<i+12;j+=2){
                        int c=Integer.parseInt(tags.get(j).trim());
                        if(c==40){double v=Double.parseDouble(tags.get(j+1).trim());if(Double.isFinite(v)&&v>0)table.globalScale=v;break;}
                        if(c==9||c==0)break;
                    }
                }
            }
            for(int i=0;i<tags.size();){
                if(Integer.parseInt(tags.get(i).trim())!=0){i+=2;continue;}
                String type=tags.get(i+1).trim();int from=i+2;i=from;
                while(i<tags.size()&&Integer.parseInt(tags.get(i).trim())!=0)i+=2;
                if("SECTION".equals(type)){section=text(tags,from,i,2,"").trim();continue;}
                if("ENDSEC".equals(type)){section="";continue;}
                if(!"TABLES".equals(section)||!"LTYPE".equals(type))continue;
                String name=text(tags,from,i,2,DxfStyle.CONTINUOUS);
                ArrayList<Double> values=new ArrayList<>();
                for(int j=from;j+1<i;j+=2)if(Integer.parseInt(tags.get(j).trim())==49)values.add(Double.parseDouble(tags.get(j+1).trim()));
                table.add(name,values);
            }
        }catch(NumberFormatException e){throw new IOException("Geçersiz DXF LTYPE tablosu",e);}
        table.add(DxfStyle.CONTINUOUS,Collections.emptyList());return table;
    }

    public static float[] dashIntervals(Pattern pattern,double scale,double deviceScale){
        if(pattern==null||pattern.elements.length==0)return null;
        ArrayList<Float> out=new ArrayList<>();double factor=Math.max(1e-6,DxfStyle.saneScale(scale)*Math.max(1e-6,deviceScale));
        for(double raw:pattern.elements){
            double len=Math.abs(raw)*factor;
            if(raw==0)len=Math.max(.75,0.02*factor);
            float v=(float)Math.max(.5,Math.min(10000d,len));out.add(v);
        }
        if(out.size()==1)out.add(out.get(0));
        if((out.size()&1)==1){int n=out.size();for(int i=0;i<n;i++)out.add(out.get(i));}
        float[] r=new float[out.size()];for(int i=0;i<r.length;i++)r[i]=out.get(i);return r;
    }

    private static String text(List<String> tags,int from,int to,int wanted,String fallback){
        for(int i=from;i+1<to;i+=2)if(Integer.parseInt(tags.get(i).trim())==wanted)return tags.get(i+1);return fallback;
    }
    private DxfLineTypes(){}
}
