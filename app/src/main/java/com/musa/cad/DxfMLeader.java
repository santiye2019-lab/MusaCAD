package com.musa.cad;

import java.util.*;

/** Minimal, deterministic decoder for the DXF MULTILEADER context emitted by LibreDWG. */
public final class DxfMLeader {
    public static final class Point {
        public final double x,y;
        Point(double x,double y){this.x=x;this.y=y;}
    }
    public static final class Result {
        public final List<List<Point>> leaderLines;
        public final String text;
        public final double textX,textY,textHeight,rotationDegrees;
        Result(List<List<Point>> lines,String text,double x,double y,double h,double r){
            leaderLines=Collections.unmodifiableList(lines);this.text=text==null?"":text;
            textX=x;textY=y;textHeight=h;rotationDegrees=r;
        }
        public boolean empty(){return leaderLines.isEmpty()&&text.trim().isEmpty();}
    }
    private DxfMLeader(){}

    public static Result parse(List<String> a,int from,int to){
        ArrayList<List<Point>> lines=new ArrayList<>();
        ArrayList<Point> current=null;
        boolean inLeader=false,inLine=false,beforeLeaders=true;
        Double nodeX=null,nodeY=null,lineX=null,textX=null,textY=null;
        double textHeight=0d,rotation=0d;
        String text="";
        for(int i=from;i+1<to;i+=2){
            int code=intOf(a.get(i));String raw=a.get(i+1)==null?"":a.get(i+1).trim();
            if(code==302&&"LEADER{".equalsIgnoreCase(raw)){inLeader=true;beforeLeaders=false;nodeX=nodeY=null;continue;}
            if(code==303&&inLeader){inLeader=false;nodeX=nodeY=null;continue;}
            if(code==304){
                if("LEADER_LINE{".equalsIgnoreCase(raw)){
                    inLine=true;current=new ArrayList<>();lineX=null;
                    if(nodeX!=null&&nodeY!=null)current.add(new Point(nodeX,nodeY));
                }else if(!raw.isEmpty()&&!raw.endsWith("{")&&!raw.equals("}")){
                    if(text.isEmpty())text=raw;
                }
                continue;
            }
            if(code==305&&inLine){
                inLine=false;lineX=null;
                if(current!=null&&current.size()>=2)lines.add(Collections.unmodifiableList(new ArrayList<>(current)));
                current=null;continue;
            }
            if(beforeLeaders){
                if(code==12&&textX==null){textX=d(raw);continue;}
                if(code==22&&textY==null){textY=d(raw);continue;}
                if((code==41||code==140)&&textHeight<=0d){double v=Math.abs(d(raw));if(v>0d)textHeight=v;continue;}
                if(code==42&&rotation==0d){rotation=Math.toDegrees(d(raw));continue;}
            }
            if(inLine){
                if(code==10){lineX=d(raw);continue;}
                if(code==20&&lineX!=null){Point p=new Point(lineX,d(raw));if(current!=null)current.add(p);lineX=null;continue;}
            }else if(inLeader){
                if(code==10){nodeX=d(raw);continue;}
                if(code==20&&nodeX!=null){nodeY=d(raw);continue;}
            }
        }
        if(inLine&&current!=null&&current.size()>=2)lines.add(Collections.unmodifiableList(new ArrayList<>(current)));
        if((textX==null||textY==null)&&!lines.isEmpty()){
            List<Point> line=lines.get(0);Point p=line.get(line.size()-1);
            if(textX==null)textX=p.x;if(textY==null)textY=p.y;
        }
        if(textHeight<=0d)textHeight=1d;
        return new Result(lines,text,textX==null?0d:textX,textY==null?0d:textY,textHeight,rotation);
    }
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double d(String s){try{return Double.parseDouble(s.trim());}catch(Exception e){return 0d;}}
}
