package com.musa.cad;

import android.graphics.PointF;
import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

/** Writes MusaCAD overlay edits into a copy of the full source/converted DXF. */
public final class DxfWriter {
    public static void write(File baseDxf,OutputStream target,DxfParser.Result drawing,List<CadEdit> edits)throws IOException{
        if(baseDxf==null||drawing==null)throw new IOException("Kaydedilecek DXF çalışma kopyası yok");
        Charset cs=DxfParser.charset(baseDxf);boolean inserted=false;String section="";boolean sectionPending=false;
        try(BufferedReader in=new BufferedReader(new InputStreamReader(new FileInputStream(baseDxf),cs),128*1024);
            BufferedWriter out=new BufferedWriter(new OutputStreamWriter(target,cs),128*1024)){
            while(true){
                FileTransfer.checkCancelled();String codeLine=in.readLine();if(codeLine==null)break;String valueLine=in.readLine();if(valueLine==null)throw new IOException("Eksik DXF etiketi");
                int code;try{code=Integer.parseInt(codeLine.trim());}catch(Exception e){throw new IOException("Geçersiz DXF etiketi",e);}String value=valueLine.trim();
                if(code==0&&"ENDSEC".equals(value)&&"ENTITIES".equals(section)&&!inserted){writeEdits(out,drawing,edits);inserted=true;}
                out.write(codeLine);out.newLine();out.write(valueLine);out.newLine();
                if(code==0&&"SECTION".equals(value)){sectionPending=true;continue;}
                if(sectionPending&&code==2){section=value.toUpperCase(Locale.ROOT);sectionPending=false;continue;}
                if(code==0&&"ENDSEC".equals(value)){section="";sectionPending=false;}
            }
            if(!inserted)throw new IOException("DXF ENTITIES bölümü bulunamadı");out.flush();
        }
    }

    private static void writeEdits(BufferedWriter out,DxfParser.Result drawing,List<CadEdit> edits)throws IOException{
        for(CadEdit edit:edits){
            switch(edit.type){
                case LINE:{PointF a=w(drawing,edit.xy[0],edit.xy[1]),b=w(drawing,edit.xy[2],edit.xy[3]);entity(out,"LINE");layer(out);n(out,10,a.x);n(out,20,a.y);n(out,30,0);n(out,11,b.x);n(out,21,b.y);n(out,31,0);break;}
                case RECTANGLE:{PointF a=w(drawing,edit.xy[0],edit.xy[1]),b=w(drawing,edit.xy[2],edit.xy[3]);entity(out,"LWPOLYLINE");layer(out);i(out,90,4);i(out,70,1);point(out,a.x,a.y);point(out,b.x,a.y);point(out,b.x,b.y);point(out,a.x,b.y);break;}
                case CIRCLE:{PointF c=w(drawing,edit.xy[0],edit.xy[1]),p=w(drawing,edit.xy[2],edit.xy[3]);entity(out,"CIRCLE");layer(out);n(out,10,c.x);n(out,20,c.y);n(out,30,0);n(out,40,Math.hypot(p.x-c.x,p.y-c.y));break;}
                case POLYLINE:{entity(out,"LWPOLYLINE");layer(out);int count=edit.xy.length/2;i(out,90,count);i(out,70,0);for(int k=0;k+1<edit.xy.length;k+=2){PointF p=w(drawing,edit.xy[k],edit.xy[k+1]);point(out,p.x,p.y);}break;}
                case TEXT:{PointF p=w(drawing,edit.xy[0],edit.xy[1]);entity(out,"TEXT");layer(out);n(out,10,p.x);n(out,20,p.y);n(out,30,0);n(out,40,drawing.worldTextHeight(30));tag(out,1,safe(edit.text));break;}
            }
        }
    }
    private static PointF w(DxfParser.Result r,float x,float y){return r.contentToWorld(x,y);}
    private static void entity(BufferedWriter o,String type)throws IOException{tag(o,0,type);}
    private static void layer(BufferedWriter o)throws IOException{tag(o,8,"0");}
    private static void point(BufferedWriter o,double x,double y)throws IOException{n(o,10,x);n(o,20,y);}
    private static void i(BufferedWriter o,int code,int v)throws IOException{tag(o,code,Integer.toString(v));}
    private static void n(BufferedWriter o,int code,double v)throws IOException{tag(o,code,String.format(Locale.US,"%.8f",v));}
    private static void tag(BufferedWriter o,int code,String v)throws IOException{o.write(String.format(Locale.US,"%3d",code));o.newLine();o.write(v==null?"":v);o.newLine();}
    private static String safe(String s){return s==null?"":s.replace("\r"," ").replace("\n"," ");}
    private DxfWriter(){}
}
