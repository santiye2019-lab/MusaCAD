package com.musa.cad;

import android.graphics.Matrix;
import android.graphics.PointF;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.*;

/** Writes MusaCAD overlay/source edits into a copy of the full source/converted DXF. */
public final class DxfWriter {
    public static void write(File baseDxf,OutputStream target,DxfParser.Result drawing,List<CadEdit> edits)throws IOException{write(baseDxf,target,drawing,edits,Collections.emptyList(),Collections.emptyList());}
    public static void write(File baseDxf,OutputStream target,DxfParser.Result drawing,List<CadEdit> edits,List<SourceRange> removedSources)throws IOException{write(baseDxf,target,drawing,edits,Collections.emptyList(),removedSources);}
    public static void write(File baseDxf,OutputStream target,DxfParser.Result drawing,List<CadEdit> additions,List<SourceReplacement> replacements,List<SourceRange> removedSources)throws IOException{
        if(baseDxf==null||drawing==null)throw new IOException("Kaydedilecek DXF çalışma kopyası yok");Matrix contentToWorld=contentToWorldMatrix(drawing);Charset cs=charset(baseDxf);boolean inserted=false;String section="";boolean sectionPending=false;int lineIndex=0;List<SourceRange> removals=removedSources==null?Collections.emptyList():removedSources;
        try(BufferedReader in=new BufferedReader(new InputStreamReader(new FileInputStream(baseDxf),cs),128*1024);BufferedWriter out=new BufferedWriter(new OutputStreamWriter(target,cs),128*1024)){
            while(true){FileTransfer.checkCancelled();String codeLine=in.readLine();if(codeLine==null)break;String valueLine=in.readLine();if(valueLine==null)throw new IOException("Eksik DXF etiketi");int code;try{code=Integer.parseInt(codeLine.trim());}catch(Exception e){throw new IOException("Geçersiz DXF etiketi",e);}String value=valueLine.trim();
                if(code==0&&"ENDSEC".equals(value)&&"ENTITIES".equals(section)&&!inserted){writeEdits(out,contentToWorld,additions,"0",null,null,null,null,drawing.activeLayout);writeReplacements(out,contentToWorld,replacements,drawing);inserted=true;}
                boolean removed=isRemoved(lineIndex,removals);if(!removed){out.write(codeLine);out.newLine();out.write(valueLine);out.newLine();}
                if(!removed){if(code==0&&"SECTION".equals(value)){sectionPending=true;lineIndex+=2;continue;}if(sectionPending&&code==2){section=value.toUpperCase(Locale.ROOT);sectionPending=false;lineIndex+=2;continue;}if(code==0&&"ENDSEC".equals(value)){section="";sectionPending=false;}}lineIndex+=2;
            }
            if(!inserted)throw new IOException("DXF ENTITIES bölümü bulunamadı");out.flush();
        }
    }

    private static boolean isRemoved(int lineIndex,List<SourceRange> removals){for(SourceRange range:removals)if(range!=null&&range.containsLine(lineIndex))return true;return false;}
    private static void writeReplacements(BufferedWriter out,Matrix contentToWorld,List<SourceReplacement> replacements,DxfParser.Result drawing)throws IOException{if(replacements==null)return;for(SourceReplacement replacement:replacements){if(replacement==null)continue;DxfParser.SourceEntity source=drawing.sourceById(replacement.sourceId);String layout=source==null?drawing.activeLayout:source.layout;writeEdit(out,contentToWorld,replacement.edit,replacement.layer,replacement.color&0x00FFFFFF,replacement.lineType,replacement.lineTypeScale,replacement.lineWeight,layout);}}
    private static void writeEdits(BufferedWriter out,Matrix contentToWorld,List<CadEdit> edits,String layer,Integer trueColor,String lineType,Double lineTypeScale,Integer lineWeight,String layout)throws IOException{if(edits==null)return;for(CadEdit edit:edits)writeEdit(out,contentToWorld,edit,layer,trueColor,lineType,lineTypeScale,lineWeight,layout);}

    private static void writeEdit(BufferedWriter out,Matrix contentToWorld,CadEdit edit,String sourceLayer,Integer trueColor,String lineType,Double lineTypeScale,Integer lineWeight,String layout)throws IOException{
        if(edit==null)return;FileTransfer.checkCancelled();String layerName=sourceLayer==null||sourceLayer.trim().isEmpty()?"0":sourceLayer;
        switch(edit.type){
            case LINE:{PointF a=w(contentToWorld,edit.xy[0],edit.xy[1]),b=w(contentToWorld,edit.xy[2],edit.xy[3]);entity(out,"LINE");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,a.x);n(out,20,a.y);n(out,30,0);n(out,11,b.x);n(out,21,b.y);n(out,31,0);break;}
            case RECTANGLE:{PointF a=w(contentToWorld,edit.xy[0],edit.xy[1]),b=w(contentToWorld,edit.xy[2],edit.xy[3]);entity(out,"LWPOLYLINE");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);i(out,90,4);i(out,70,1);point(out,a.x,a.y);point(out,b.x,a.y);point(out,b.x,b.y);point(out,a.x,b.y);break;}
            case CIRCLE:{PointF c=w(contentToWorld,edit.xy[0],edit.xy[1]),p=w(contentToWorld,edit.xy[2],edit.xy[3]);entity(out,"CIRCLE");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,c.x);n(out,20,c.y);n(out,30,0);n(out,40,Math.hypot(p.x-c.x,p.y-c.y));break;}
            case ARC:{if(edit.xy.length<8)break;PointF a=w(contentToWorld,edit.xy[0],edit.xy[1]),m=w(contentToWorld,edit.xy[2],edit.xy[3]),b=w(contentToWorld,edit.xy[4],edit.xy[5]),c=w(contentToWorld,edit.xy[6],edit.xy[7]);double sa=angle(c,a),ma=angle(c,m),ea=angle(c,b);double start=sa,end=ea;if(!onCcw(sa,ea,ma)){start=ea;end=sa;}entity(out,"ARC");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,c.x);n(out,20,c.y);n(out,30,0);n(out,40,Math.hypot(a.x-c.x,a.y-c.y));n(out,50,start);n(out,51,end);break;}
            case ELLIPSE:{if(edit.xy.length<6)break;PointF c=w(contentToWorld,edit.xy[0],edit.xy[1]),a=w(contentToWorld,edit.xy[2],edit.xy[3]),b=w(contentToWorld,edit.xy[4],edit.xy[5]);double ax=a.x-c.x,ay=a.y-c.y,bx=b.x-c.x,by=b.y-c.y,major=Math.hypot(ax,ay),minor=Math.hypot(bx,by);if(major<1e-9||minor<1e-9)break;if(minor>major){double tx=ax,ty=ay;ax=bx;ay=by;bx=tx;by=ty;double t=major;major=minor;minor=t;}entity(out,"ELLIPSE");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,c.x);n(out,20,c.y);n(out,30,0);n(out,11,ax);n(out,21,ay);n(out,31,0);n(out,40,minor/major);n(out,41,0);n(out,42,Math.PI*2d);break;}
            case POINT:{if(edit.xy.length<2)break;PointF p=w(contentToWorld,edit.xy[0],edit.xy[1]);entity(out,"POINT");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,p.x);n(out,20,p.y);n(out,30,0);break;}
            case XLINE:{if(edit.xy.length<4)break;PointF a=w(contentToWorld,edit.xy[0],edit.xy[1]),b=w(contentToWorld,edit.xy[2],edit.xy[3]);double dx=b.x-a.x,dy=b.y-a.y,len=Math.hypot(dx,dy);if(len<1e-9)break;entity(out,"XLINE");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,a.x);n(out,20,a.y);n(out,30,0);n(out,11,dx/len);n(out,21,dy/len);n(out,31,0);break;}
            case POLYLINE:{entity(out,"LWPOLYLINE");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);int count=edit.xy.length/2;i(out,90,count);i(out,70,edit.closed?1:0);for(int k=0;k+1<edit.xy.length;k+=2){PointF p=w(contentToWorld,edit.xy[k],edit.xy[k+1]);point(out,p.x,p.y);}break;}
            case HATCH:{if(edit.xy.length<6)break;String pattern="ANSI31".equalsIgnoreCase(edit.text)?"ANSI31":"SOLID";boolean solid="SOLID".equals(pattern);entity(out,"HATCH");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);
                n(out,10,0);n(out,20,0);n(out,30,0);n(out,210,0);n(out,220,0);n(out,230,1);tag(out,2,pattern);i(out,70,solid?1:0);i(out,71,0);
                i(out,91,1);i(out,92,2);i(out,72,0);i(out,73,1);int count=edit.xy.length/2;i(out,93,count);
                for(int k=0;k+1<edit.xy.length;k+=2){PointF p=w(contentToWorld,edit.xy[k],edit.xy[k+1]);point(out,p.x,p.y);}i(out,97,0);
                i(out,75,0);i(out,76,1);n(out,52,edit.rotationDegrees);n(out,41,Math.max(.001,edit.textHeight));i(out,77,0);
                if(solid){i(out,78,0);}else{i(out,78,1);n(out,53,45d+edit.rotationDegrees);n(out,43,0);n(out,44,0);n(out,45,0);n(out,46,1);i(out,79,0);}
                i(out,98,0);break;}
            case TEXT:{PointF p=w(contentToWorld,edit.xy[0],edit.xy[1]);entity(out,"TEXT");common(out,layerName,trueColor,lineType,lineTypeScale,lineWeight,layout);n(out,10,p.x);n(out,20,p.y);n(out,30,0);double h=worldTextHeight(contentToWorld,edit.hasTextStyle()?edit.textHeight:30f);n(out,40,h);n(out,50,-edit.rotationDegrees);if(edit.hasTextStyle()){tag(out,7,edit.textStyleName);if(Math.abs(edit.textWidthFactor-1f)>1e-6)n(out,41,edit.textWidthFactor);if(Math.abs(edit.textOblique)>1e-6)n(out,51,edit.textOblique);if(edit.textGenerationFlags!=0)i(out,71,edit.textGenerationFlags);}tag(out,1,safe(edit.text));break;}
        }
    }

    /** Result keeps the world-to-content transform private; keep persistence isolated here rather than exposing parser internals. */
    private static Matrix contentToWorldMatrix(DxfParser.Result drawing)throws IOException{try{Field field=DxfParser.Result.class.getDeclaredField("view");field.setAccessible(true);Matrix worldToContent=(Matrix)field.get(drawing);Matrix inverse=new Matrix();if(worldToContent==null||!worldToContent.invert(inverse))throw new IOException("DXF koordinat dönüşümü oluşturulamadı");return inverse;}catch(ReflectiveOperationException|SecurityException e){throw new IOException("DXF koordinat dönüşümüne erişilemedi",e);}}
    private static Charset charset(File file)throws IOException{String header;try(InputStream in=new FileInputStream(file)){byte[]bytes=new byte[65536];int count=in.read(bytes);header=new String(bytes,0,Math.max(0,count),StandardCharsets.ISO_8859_1);}java.util.regex.Matcher version=java.util.regex.Pattern.compile("AC10([0-9]{2})").matcher(header);if(version.find()&&Integer.parseInt(version.group(1))>=21)return StandardCharsets.UTF_8;java.util.regex.Matcher cp=java.util.regex.Pattern.compile("ANSI_([0-9]+)").matcher(header);try{if(cp.find())return Charset.forName("windows-"+cp.group(1));}catch(Exception ignored){}return Charset.forName("windows-1252");}
    private static double worldTextHeight(Matrix contentToWorld,float contentPixels){float[]vector={0,contentPixels};contentToWorld.mapVectors(vector);return Math.max(.001,Math.hypot(vector[0],vector[1]));}
    private static PointF w(Matrix m,float x,float y){float[]xy={x,y};m.mapPoints(xy);return new PointF(xy[0],xy[1]);}
    private static double angle(PointF c,PointF p){double a=Math.toDegrees(Math.atan2(p.y-c.y,p.x-c.x));return a<0d?a+360d:a;}
    private static double ccw(double from,double to){double d=to-from;while(d<0d)d+=360d;while(d>=360d)d-=360d;return d;}
    private static boolean onCcw(double start,double end,double point){return ccw(start,point)<=ccw(start,end)+1e-7;}
    private static void entity(BufferedWriter o,String type)throws IOException{tag(o,0,type);}
    private static void common(BufferedWriter o,String layer,Integer trueColor,String lineType,Double lineTypeScale,Integer lineWeight,String layout)throws IOException{tag(o,8,layer);if(layout!=null&&!layout.trim().isEmpty()&&!DxfBlocks.MODEL_LAYOUT.equalsIgnoreCase(layout.trim())){i(o,67,1);tag(o,410,layout.trim());}if(trueColor!=null)tag(o,420,Integer.toString(trueColor));if(lineType!=null&&!lineType.trim().isEmpty())tag(o,6,DxfLineStyle.normalizeName(lineType));if(lineTypeScale!=null&&Double.isFinite(lineTypeScale)&&lineTypeScale>0d&&Math.abs(lineTypeScale-1d)>1e-9)n(o,48,lineTypeScale);if(lineWeight!=null)i(o,370,DxfLineStyle.normalizeWeight(lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT));}
    private static void point(BufferedWriter o,double x,double y)throws IOException{n(o,10,x);n(o,20,y);}private static void i(BufferedWriter o,int code,int v)throws IOException{tag(o,code,Integer.toString(v));}private static void n(BufferedWriter o,int code,double v)throws IOException{tag(o,code,String.format(Locale.US,"%.8f",v));}private static void tag(BufferedWriter o,int code,String v)throws IOException{o.write(String.format(Locale.US,"%3d",code));o.newLine();o.write(v==null?"":v);o.newLine();}private static String safe(String s){return s==null?"":s.replace("\r"," ").replace("\n"," ");}
    private DxfWriter(){}
}
