package com.musa.cad;

import java.util.ArrayList;
import java.util.List;

/** Minimal, geometry-only parser for the nested DXF MULTILEADER context records. */
public final class DxfMLeader {
    public static final class Leader {
        public final double[] points;
        public final double arrowSize;
        Leader(double[] points,double arrowSize){this.points=points;this.arrowSize=arrowSize;}
    }
    public static final class Data {
        public final String text;
        public final double textX,textY,textHeight;
        public final List<Leader> leaders;
        Data(String text,double textX,double textY,double textHeight,List<Leader> leaders){
            this.text=text==null?"":text;this.textX=textX;this.textY=textY;this.textHeight=textHeight;this.leaders=leaders;
        }
    }

    /** Parses an alternating code/value DXF tag list without any Android dependencies. */
    public static Data parse(List<String> tags){
        String text="";double textX=Double.NaN,textY=Double.NaN,textHeight=0;
        boolean context=false,inLeader=false,inLine=false;double pendingX=Double.NaN;
        double landingX=Double.NaN,landingY=Double.NaN,leaderArrow=0;
        ArrayList<Double> line=new ArrayList<>();ArrayList<Leader> leaders=new ArrayList<>();
        if(tags==null)return new Data("",textX,textY,textHeight,leaders);
        for(int i=0;i+1<tags.size();i+=2){
            int code=code(tags.get(i));String value=tags.get(i+1)==null?"":tags.get(i+1);
            String marker=value.trim();
            if(code==300&&marker.startsWith("CONTEXT_DATA")){context=true;continue;}
            if(code==301&&"}".equals(marker)){finishLine(line,leaderArrow,leaders);break;}
            if(code==302&&marker.startsWith("LEADER")){finishLine(line,leaderArrow,leaders);inLeader=true;inLine=false;landingX=landingY=Double.NaN;leaderArrow=0;pendingX=Double.NaN;continue;}
            if(code==303&&"}".equals(marker)){finishLine(line,leaderArrow,leaders);inLeader=false;inLine=false;pendingX=Double.NaN;continue;}
            if(code==304&&marker.startsWith("LEADER_LINE")){finishLine(line,leaderArrow,leaders);inLine=true;pendingX=Double.NaN;
                if(finite(landingX,landingY)){line.add(landingX);line.add(landingY);}continue;}
            if(code==305&&"}".equals(marker)){finishLine(line,leaderArrow,leaders);inLine=false;pendingX=Double.NaN;continue;}

            if(context&&!inLeader&&code==304&&!structural(marker)&&text.isEmpty())text=value;
            if(context&&!inLeader&&code==10&&Double.isNaN(textX)){textX=number(value);continue;}
            if(context&&!inLeader&&code==20&&Double.isNaN(textY)){textY=number(value);continue;}
            if(context&&!inLeader&&code==41&&!(textHeight>0)){double n=number(value);if(n>0)textHeight=n;continue;}

            if(inLeader&&!inLine){
                if(code==10&&Double.isNaN(landingX)){landingX=number(value);continue;}
                if(code==20&&Double.isNaN(landingY)){landingY=number(value);continue;}
                if(code==40&&!(leaderArrow>0)){double n=number(value);if(n>0)leaderArrow=n;continue;}
            }
            if(inLine){
                if(code==10){pendingX=number(value);continue;}
                if(code==20&&Double.isFinite(pendingX)){double y=number(value);if(Double.isFinite(y)){line.add(pendingX);line.add(y);}pendingX=Double.NaN;}
            }
        }
        finishLine(line,leaderArrow,leaders);
        return new Data(text,textX,textY,textHeight,leaders);
    }

    private static boolean structural(String value){return value.startsWith("LEADER_LINE")||"}".equals(value)||value.startsWith("CONTEXT_DATA")||value.startsWith("LEADER{");}
    private static void finishLine(ArrayList<Double> values,double arrowSize,ArrayList<Leader> leaders){
        if(values.size()>=4){
            // Consecutive duplicated landing/first vertices add no geometry and confuse arrow direction.
            ArrayList<Double> clean=new ArrayList<>();
            for(int i=0;i+1<values.size();i+=2){double x=values.get(i),y=values.get(i+1);int n=clean.size();
                if(n>=2&&Math.hypot(x-clean.get(n-2),y-clean.get(n-1))<1e-10)continue;clean.add(x);clean.add(y);}
            if(clean.size()>=4){double[] p=new double[clean.size()];for(int i=0;i<p.length;i++)p[i]=clean.get(i);leaders.add(new Leader(p,arrowSize));}
        }
        values.clear();
    }
    private static int code(String raw){try{return Integer.parseInt(raw.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double number(String raw){try{double n=Double.parseDouble(raw.trim());return Double.isFinite(n)?n:Double.NaN;}catch(Exception e){return Double.NaN;}}
    private static boolean finite(double a,double b){return Double.isFinite(a)&&Double.isFinite(b);}
    private DxfMLeader(){}
}
