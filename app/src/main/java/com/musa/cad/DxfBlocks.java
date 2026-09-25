package com.musa.cad;

import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

/** Expands ordinary 2D INSERTs/dimension picture blocks and resolves DXF display properties. */
public final class DxfBlocks {
    public static final String MODEL_LAYOUT="Model";
    public static final String PAPER_LAYOUT="Paper Space";

    public static final class Transform {
        public final double a,b,c,d,x,y;
        public Transform(){this(1,0,0,1,0,0);}
        private Transform(double a,double b,double c,double d,double x,double y){this.a=a;this.b=b;this.c=c;this.d=d;this.x=x;this.y=y;}
        public double[] point(double px,double py){return new double[]{a*px+c*py+x,b*px+d*py+y};}
        public double scale(){double sx=Math.hypot(a,b),sy=Math.hypot(c,d);double value=(sx+sy)/2d;return Double.isFinite(value)&&value>0d?value:1d;}
        public boolean isIdentity(){return Math.abs(a-1d)<1e-12&&Math.abs(d-1d)<1e-12&&Math.abs(b)<1e-12&&Math.abs(c)<1e-12&&Math.abs(x)<1e-12&&Math.abs(y)<1e-12;}
        public Transform thenLocal(Transform t){return new Transform(a*t.a+c*t.b,b*t.a+d*t.b,a*t.c+c*t.d,b*t.c+d*t.d,a*t.x+c*t.y+x,b*t.x+d*t.y+y);}
        static Transform translate(double x,double y){return new Transform(1,0,0,1,x,y);}
        static Transform insert(double bx,double by,double sx,double sy,double degrees,double x,double y){double r=Math.toRadians(degrees),co=Math.cos(r),si=Math.sin(r);double a=co*sx,b=si*sx,c=-si*sy,d=co*sy;return new Transform(a,b,c,d,x-a*bx-c*by,y-b*bx-d*by);}
    }

    public static final class Record {
        public final String type;public final int from,to,sourceFrom,sourceStart,sourceTo;final List<String>tags;
        Record(String type,List<String>tags,int from,int to){this(type,tags,from,to,from,Math.max(0,from-2),to);}
        Record(String type,List<String>tags,int from,int to,int sourceFrom,int sourceStart,int sourceTo){
            this.type=type;this.tags=tags;this.from=from;this.to=to;this.sourceFrom=sourceFrom;this.sourceStart=sourceStart;this.sourceTo=sourceTo;
        }
        List<String> tags(){return tags;}
        String text(int code,String fallback){for(int i=from;i+1<to;i+=2)if(Integer.parseInt(tags.get(i).trim())==code)return tags.get(i+1).trim();return fallback;}
        boolean has(int code){for(int i=from;i+1<to;i+=2)if(Integer.parseInt(tags.get(i).trim())==code)return true;return false;}
        double number(int code,double fallback)throws IOException{try{double v=Double.parseDouble(text(code,Double.toString(fallback)));if(!Double.isFinite(v))throw new NumberFormatException();return v;}catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}}
        int integer(int code,int fallback)throws IOException{try{return Integer.parseInt(text(code,Integer.toString(fallback)).trim());}catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}}
        double[] numbers(int code)throws IOException{ArrayList<Double>values=new ArrayList<>();try{for(int i=from;i+1<to;i+=2)if(Integer.parseInt(tags.get(i).trim())==code){double v=Double.parseDouble(tags.get(i+1).trim());if(!Double.isFinite(v))throw new NumberFormatException();values.add(v);}}catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}double[]result=new double[values.size()];for(int i=0;i<result.length;i++)result[i]=values.get(i);return result;}
    }

    public static final class Placement {
        public final Record record;public final Transform transform;public final String layer,layout;public final int color;public final String lineType;public final double lineTypeScale;public final int lineWeight;public final double blockScale;public final boolean directRoot;
        Placement(Record record,Transform transform,String layer,String layout,int color,String lineType,double lineTypeScale,int lineWeight,boolean directRoot){this.record=record;this.transform=transform;this.layer=layer;this.layout=normalizeLayout(layout);this.color=color;this.lineType=DxfLineStyle.normalizeName(lineType);this.lineTypeScale=lineTypeScale;this.lineWeight=lineWeight;this.blockScale=transform.scale();this.directRoot=directRoot;}
    }

    private static final class Block{
        final Record header;final List<Record>members=new ArrayList<>();
        File file;Charset charset;long offset;int offsetLine;
        Block(Record header){this.header=header;}
    }

    public interface Sink{
        default void begin(Result metadata)throws IOException{}
        void accept(Placement placement)throws IOException;
        default void finish()throws IOException{}
    }

    public static final class Result {
        public final List<Placement>placements=new ArrayList<>();public final Map<String,Integer>layerColors=new LinkedHashMap<>();public final Map<String,String>layerLineTypes=new LinkedHashMap<>();public final Map<String,Integer>layerLineWeights=new LinkedHashMap<>();public final Map<String,DxfLineStyle.Pattern>lineTypes=new LinkedHashMap<>();public final Map<String,DxfTextStyle.Style>textStyles=new LinkedHashMap<>();public final Set<String>layoutNames=new LinkedHashSet<>();
        public int skipped,units;public float[] modelExtents;public int defaultLineweight=DxfLineStyle.DEFAULT_LINEWEIGHT;public double globalLineTypeScale=1d;private int visits;private Sink sink;
    }

    public static Result expand(List<String>tags)throws IOException{return expand(tags,DxfLineStyle.DEFAULT_LINEWEIGHT);}
    public static Result expand(List<String>tags,int defaultLineweight)throws IOException{
        List<Record>records=new ArrayList<>();
        try{if(tags.size()%2!=0)throw new NumberFormatException();for(int i=0;i<tags.size();i+=2)Integer.parseInt(tags.get(i).trim());for(int i=0;i<tags.size();){if(Integer.parseInt(tags.get(i).trim())!=0){i+=2;continue;}String type=tags.get(i+1).trim();int from=i+2;i=from;while(i<tags.size()&&Integer.parseInt(tags.get(i).trim())!=0)i+=2;records.add(new Record(type,tags,from,i));}}catch(NumberFormatException e){throw new IOException("Geçersiz ASCII DXF etiketleri",e);}
        int defaultWeight=DxfLineStyle.normalizeWeight(defaultLineweight,DxfLineStyle.DEFAULT_LINEWEIGHT);Map<String,Block>blocks=new HashMap<>();List<Record>roots=new ArrayList<>();Map<String,Integer>layerColorLookup=new HashMap<>();Map<String,String>layerLineTypeLookup=new HashMap<>();Map<String,Integer>layerLineWeightLookup=new HashMap<>();Result result=new Result();result.lineTypes.put(DxfLineStyle.CONTINUOUS,new DxfLineStyle.Pattern(DxfLineStyle.CONTINUOUS,new double[0],false));String section="";Block active=null;
        for(Record record:records){
            if(record.type.equals("SECTION")){section=record.text(2,"");active=null;continue;}if(record.type.equals("ENDSEC")){section="";active=null;continue;}
            if(section.equals("TABLES")&&record.type.equals("LTYPE")){String name=DxfLineStyle.normalizeName(record.text(2,DxfLineStyle.CONTINUOUS));result.lineTypes.put(name,DxfLineTypeParser.parse(name,tags,record.from,record.to));}
            else if(section.equals("TABLES")&&record.type.equals("LAYER")){String name=record.text(2,"0"),lookup=key(name);int color=layerColor(record);String lineType=DxfLineStyle.normalizeName(record.text(6,DxfLineStyle.CONTINUOUS));int lineWeight=layerLineweight(record,defaultWeight);layerColorLookup.put(lookup,color);layerLineTypeLookup.put(lookup,lineType);layerLineWeightLookup.put(lookup,lineWeight);result.layerColors.put(name,color);result.layerLineTypes.put(name,lineType);result.layerLineWeights.put(name,lineWeight);}
            else if(section.equals("BLOCKS")){if(record.type.equals("BLOCK")){active=new Block(record);blocks.put(key(record.text(2,"")),active);}else if(record.type.equals("ENDBLK"))active=null;else if(active!=null)active.members.add(record);}
            else if(section.equals("ENTITIES"))roots.add(record);
        }
        if(!layerColorLookup.containsKey("0")){int color=DxfColor.aciArgb(DxfColor.DEFAULT_ACI);layerColorLookup.put("0",color);layerLineTypeLookup.put("0",DxfLineStyle.CONTINUOUS);layerLineWeightLookup.put("0",defaultWeight);result.layerColors.put("0",color);result.layerLineTypes.put("0",DxfLineStyle.CONTINUOUS);result.layerLineWeights.put("0",defaultWeight);}
        int defaultBlockColor=DxfColor.aciArgb(DxfColor.DEFAULT_ACI);for(Record root:roots)expand(root,new Transform(),"0",MODEL_LAYOUT,defaultBlockColor,DxfLineStyle.CONTINUOUS,defaultWeight,1d,true,blocks,layerColorLookup,layerLineTypeLookup,layerLineWeightLookup,defaultWeight,new HashSet<>(),result);if(result.layoutNames.isEmpty())result.layoutNames.add(MODEL_LAYOUT);return result;
    }


    /**
     * Streaming expansion for large DXF files. Raw drawing text is never retained globally.
     * TABLES/BLOCKS are indexed in pass one; ENTITIES are expanded record-by-record in pass two.
     */
    public static Result expand(File file,Charset charset,Sink sink)throws IOException{
        if(file==null||sink==null)throw new IOException("Streaming DXF girişi eksik");
        Map<String,Block>blocks=new HashMap<>();Map<String,Integer>layerColorLookup=new HashMap<>();Map<String,String>layerLineTypeLookup=new HashMap<>();Map<String,Integer>layerLineWeightLookup=new HashMap<>();
        Result result=new Result();result.lineTypes.put(DxfLineStyle.CONTINUOUS,new DxfLineStyle.Pattern(DxfLineStyle.CONTINUOUS,new double[0],false));result.textStyles.put(DxfTextStyle.STANDARD,DxfTextStyle.defaultStyle());
        String section="";boolean eof=false;
        try(DxfStream input=new DxfStream(file,charset)){
            Record r;
            while((r=input.next())!=null){
                FileTransfer.checkCancelled();
                if("SECTION".equals(r.type)){
                    section=r.text(2,"");
                    if("HEADER".equals(section)){
                        result.units=headerInt(r,"$INSUNITS",70,0);
                        result.defaultLineweight=DxfLineStyle.normalizeWeight(headerInt(r,"$LWDEFAULT",370,DxfLineStyle.DEFAULT_LINEWEIGHT),DxfLineStyle.DEFAULT_LINEWEIGHT);
                        result.globalLineTypeScale=safeScale(headerDouble(r,"$LTSCALE",40,1d));
                        double x0=headerDouble(r,"$EXTMIN",10,Double.NaN),y0=headerDouble(r,"$EXTMIN",20,Double.NaN);
                        double x1=headerDouble(r,"$EXTMAX",10,Double.NaN),y1=headerDouble(r,"$EXTMAX",20,Double.NaN);
                        if(Double.isFinite(x0)&&Double.isFinite(y0)&&Double.isFinite(x1)&&Double.isFinite(y1)&&x1>x0&&y1>y0)
                            result.modelExtents=new float[]{(float)x0,(float)y0,(float)x1,(float)y1};
                    }
                    continue;
                }
                if("ENDSEC".equals(r.type)){section="";continue;}
                if("EOF".equals(r.type)){eof=true;break;}
                if("TABLES".equals(section)&&"LTYPE".equals(r.type)){
                    String name=DxfLineStyle.normalizeName(r.text(2,DxfLineStyle.CONTINUOUS));
                    result.lineTypes.put(name,DxfLineTypeParser.parse(name,r.tags,r.from,r.to));
                }else if("TABLES".equals(section)&&"LAYER".equals(r.type)){
                    String name=r.text(2,"0"),lookup=key(name);int color=layerColor(r);String lineType=DxfLineStyle.normalizeName(r.text(6,DxfLineStyle.CONTINUOUS));int lineWeight=layerLineweight(r,result.defaultLineweight);
                    layerColorLookup.put(lookup,color);layerLineTypeLookup.put(lookup,lineType);layerLineWeightLookup.put(lookup,lineWeight);result.layerColors.put(name,color);result.layerLineTypes.put(name,lineType);result.layerLineWeights.put(name,lineWeight);
                }else if("TABLES".equals(section)&&"STYLE".equals(r.type)){
                    ArrayList<String> one=new ArrayList<>(r.tags.size()+2);one.add("0");one.add("STYLE");one.addAll(r.tags);
                    String styleName=DxfTextStyle.normalize(r.text(2,DxfTextStyle.STANDARD));DxfTextStyle.Style style=DxfTextStyle.parse(one).get(styleName);if(style!=null)result.textStyles.put(styleName,style);
                }else if("BLOCKS".equals(section)&&"BLOCK".equals(r.type)){
                    if(blocks.size()>=100000)throw new IOException("DXF blok sayısı sınırı aşıldı");
                    Block block=new Block(r);block.file=file;block.charset=charset;block.offset=input.nextRecordOffset;block.offsetLine=input.nextRecordLine;blocks.put(key(r.text(2,"")),block);
                }
            }
        }
        if(!eof)throw new IOException("DXF dosyası tamamlanmamış (EOF yok)");
        ensureLayerZero(result,layerColorLookup,layerLineTypeLookup,layerLineWeightLookup);
        result.sink=sink;sink.begin(result);result.visits=0;
        int defaultBlockColor=DxfColor.aciArgb(DxfColor.DEFAULT_ACI);
        try(DxfStream input=new DxfStream(file,charset)){
            section="";Record r;Transform identity=new Transform();Set<String>stack=new HashSet<>();
            while((r=input.next())!=null){
                FileTransfer.checkCancelled();
                if("SECTION".equals(r.type)){section=r.text(2,"");continue;}
                if("ENDSEC".equals(r.type)){section="";continue;}
                if("EOF".equals(r.type))break;
                if("ENTITIES".equals(section))expand(r,identity,"0",MODEL_LAYOUT,defaultBlockColor,DxfLineStyle.CONTINUOUS,result.defaultLineweight,1d,true,blocks,layerColorLookup,layerLineTypeLookup,layerLineWeightLookup,result.defaultLineweight,stack,result);
            }
        }finally{
            try{sink.finish();}finally{result.sink=null;}
        }
        if(result.layoutNames.isEmpty())result.layoutNames.add(MODEL_LAYOUT);
        return result;
    }

    private static void ensureLayerZero(Result result,Map<String,Integer>colors,Map<String,String>types,Map<String,Integer>weights){
        if(colors.containsKey("0"))return;int color=DxfColor.aciArgb(DxfColor.DEFAULT_ACI);
        colors.put("0",color);types.put("0",DxfLineStyle.CONTINUOUS);weights.put("0",result.defaultLineweight);
        result.layerColors.put("0",color);result.layerLineTypes.put("0",DxfLineStyle.CONTINUOUS);result.layerLineWeights.put("0",result.defaultLineweight);
    }
    private static int headerInt(Record r,String variable,int wanted,int fallback){
        for(int i=r.from;i+1<r.to;i+=2){if(code(r.tags.get(i))!=9||!variable.equals(r.tags.get(i+1).trim()))continue;for(int j=i+2;j+1<r.to;j+=2){int c=code(r.tags.get(j));if(c==9)break;if(c==wanted){try{return Integer.parseInt(r.tags.get(j+1).trim());}catch(Exception ignored){return fallback;}}}}return fallback;
    }
    private static double headerDouble(Record r,String variable,int wanted,double fallback){
        for(int i=r.from;i+1<r.to;i+=2){if(code(r.tags.get(i))!=9||!variable.equals(r.tags.get(i+1).trim()))continue;for(int j=i+2;j+1<r.to;j+=2){int c=code(r.tags.get(j));if(c==9)break;if(c==wanted){try{double v=Double.parseDouble(r.tags.get(j+1).trim());return Double.isFinite(v)?v:fallback;}catch(Exception ignored){return fallback;}}}}return fallback;
    }
    private static int code(String value){try{return Integer.parseInt(value.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double safeScale(double value){return Double.isFinite(value)&&value>0d?value:1d;}
    private static void emit(Result result,Placement placement)throws IOException{
        result.layoutNames.add(placement.layout);if(result.sink!=null)result.sink.accept(placement);else result.placements.add(placement);
    }
    private static void expandMembers(Block block,Transform transform,String layer,String layout,int color,String lineType,int lineWeight,double lineScale,Map<String,Block>blocks,Map<String,Integer>layerColors,Map<String,String>layerLineTypes,Map<String,Integer>layerLineWeights,int defaultLineweight,Set<String>stack,Result result)throws IOException{
        if(block.file==null){for(Record member:block.members)expand(member,transform,layer,layout,color,lineType,lineWeight,lineScale,false,blocks,layerColors,layerLineTypes,layerLineWeights,defaultLineweight,stack,result);return;}
        try(DxfStream input=new DxfStream(block.file,block.charset)){
            input.seek(block.offset,block.offsetLine);Record member;boolean ended=false;
            while((member=input.next())!=null){
                if("ENDBLK".equals(member.type)){ended=true;break;}
                if("ENDSEC".equals(member.type)||"EOF".equals(member.type)||"BLOCK".equals(member.type))break;
                expand(member,transform,layer,layout,color,lineType,lineWeight,lineScale,false,blocks,layerColors,layerLineTypes,layerLineWeights,defaultLineweight,stack,result);
            }
            if(!ended)throw new IOException("DXF blok sonu bulunamadı");
        }
    }

    private static void expand(Record r,Transform parent,String parentLayer,String parentLayout,int parentBlockColor,String parentBlockLineType,int parentBlockLineWeight,double parentLineTypeScale,boolean directRoot,Map<String,Block>blocks,Map<String,Integer>layerColors,Map<String,String>layerLineTypes,Map<String,Integer>layerLineWeights,int defaultLineweight,Set<String>stack,Result result)throws IOException{
        if(Thread.currentThread().isInterrupted())throw new java.io.InterruptedIOException("Yükleme iptal edildi");if(++result.visits>2000000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");if(r.type.equals("SEQEND"))return;
        String layer=r.text(8,"0");if(layer.equals("0"))layer=parentLayer;String layout=entityLayout(r,parentLayout);int color=entityColor(r,layer,parentBlockColor,layerColors);String lineType=entityLineType(r,layer,parentBlockLineType,layerLineTypes);int lineWeight=entityLineweight(r,layer,parentBlockLineWeight,layerLineWeights,defaultLineweight);double ownScale=r.number(48,1d);if(!Double.isFinite(ownScale)||ownScale<=0d)ownScale=1d;double effectiveLineTypeScale=parentLineTypeScale*ownScale;
        if(r.type.equals("DIMENSION")){
            String name=key(r.text(2,""));Block block=blocks.get(name);boolean defaultExtrusion=r.number(210,0)==0&&r.number(220,0)==0&&r.number(230,1)==1;
            if(block!=null&&!stack.contains(name)&&stack.size()<32&&defaultExtrusion){Transform transform=parent.thenLocal(Transform.translate(r.number(12,0),r.number(22,0)));stack.add(name);expandMembers(block,transform,layer,layout,color,lineType,lineWeight,effectiveLineTypeScale,blocks,layerColors,layerLineTypes,layerLineWeights,defaultLineweight,stack,result);stack.remove(name);return;}
        }
        if(!r.type.equals("INSERT")){emit(result,new Placement(r,parent,layer,layout,color,lineType,effectiveLineTypeScale,lineWeight,directRoot));return;}
        String name=key(r.text(2,""));Block block=blocks.get(name);if(block==null||stack.contains(name)||stack.size()>=32){result.skipped++;return;}
        double nx=r.number(210,0),ny=r.number(220,0),nz=r.number(230,1);
        boolean positiveZ=Math.abs(nx)<1e-8&&Math.abs(ny)<1e-8&&nz>.999999;
        boolean negativeZ=Math.abs(nx)<1e-8&&Math.abs(ny)<1e-8&&nz<-.999999;
        // 2D MusaCAD rendering deliberately ignores entity/block elevation (group 30);
        // elevation must not make ordinary plan blocks disappear. Only non-planar OCS is skipped.
        if(r.number(70,1)!=1||r.number(71,1)!=1||(!positiveZ&&!negativeZ)||(((int)block.header.number(70,0))&12)!=0||!block.header.text(1,"").isEmpty()){result.skipped++;return;}
        double sx=r.number(41,1),sy=r.number(42,1);if(sx==0||sy==0){result.skipped++;return;}
        double bx=block.header.number(10,0),by=block.header.number(20,0),degrees=r.number(50,0),ix=r.number(10,0),iy=r.number(20,0);
        // For OCS normal (0,0,-1), OCS X maps to -WCS X and rotation reverses.
        Transform local=negativeZ
            ?Transform.insert(bx,by,-sx,sy,-degrees,-ix,iy)
            :Transform.insert(bx,by,sx,sy,degrees,ix,iy);
        Transform transform=parent.thenLocal(local);stack.add(name);expandMembers(block,transform,layer,layout,color,lineType,lineWeight,effectiveLineTypeScale,blocks,layerColors,layerLineTypes,layerLineWeights,defaultLineweight,stack,result);stack.remove(name);
    }

    private static String entityLayout(Record record,String inherited)throws IOException{
        String explicit=record.text(410,"").trim();if(!explicit.isEmpty())return normalizeLayout(explicit);
        if(record.has(67))return record.integer(67,0)==1?(isPaper(inherited)?normalizeLayout(inherited):PAPER_LAYOUT):MODEL_LAYOUT;
        return normalizeLayout(inherited);
    }
    private static boolean isPaper(String layout){return layout!=null&&!normalizeLayout(layout).equalsIgnoreCase(MODEL_LAYOUT);}
    private static String normalizeLayout(String layout){String value=layout==null?"":layout.trim();return value.isEmpty()?MODEL_LAYOUT:value;}
    private static int trueColor(Record record)throws IOException{
        try{
            long raw=Long.parseLong(record.text(420,"0").trim());
            if(raw<0L||raw>0xFFFFFFFFL)throw new NumberFormatException();
            return DxfColor.trueColorArgb((int)raw);
        }catch(NumberFormatException e){throw new IOException("Geçersiz DXF TrueColor değeri",e);}
    }
    private static int layerColor(Record record)throws IOException{if(record.has(420))return trueColor(record);int aci=Math.abs(record.integer(62,DxfColor.DEFAULT_ACI));return DxfColor.aciArgb(aci>=1&&aci<=255?aci:DxfColor.DEFAULT_ACI);}
    private static int layerLineweight(Record record,int defaultWeight)throws IOException{return DxfLineStyle.normalizeWeight(record.integer(370,DxfLineStyle.LW_DEFAULT),defaultWeight);}
    private static int entityColor(Record record,String layer,int inheritedBlockColor,Map<String,Integer>layerColors)throws IOException{if(record.has(420))return trueColor(record);int aci=record.integer(62,DxfColor.BYLAYER);if(aci==DxfColor.BYBLOCK)return inheritedBlockColor;if(aci==DxfColor.BYLAYER)return layerColors.getOrDefault(key(layer),DxfColor.aciArgb(DxfColor.DEFAULT_ACI));aci=Math.abs(aci);if(aci>=1&&aci<=255)return DxfColor.aciArgb(aci);return layerColors.getOrDefault(key(layer),DxfColor.aciArgb(DxfColor.DEFAULT_ACI));}
    private static String entityLineType(Record record,String layer,String inheritedBlockType,Map<String,String>layerLineTypes){String layerType=layerLineTypes.getOrDefault(key(layer),DxfLineStyle.CONTINUOUS);return DxfLineStyle.resolveLinetype(record.text(6,DxfLineStyle.BYLAYER),layerType,inheritedBlockType);}
    private static int entityLineweight(Record record,String layer,int inheritedBlockWeight,Map<String,Integer>layerLineWeights,int defaultWeight)throws IOException{int layerWeight=layerLineWeights.getOrDefault(key(layer),defaultWeight);return DxfLineStyle.resolveLineweight(record.integer(370,DxfLineStyle.LW_BYLAYER),layerWeight,inheritedBlockWeight,defaultWeight);}
    private static String key(String s){return s.toUpperCase(Locale.ROOT);}
    private DxfBlocks(){}
}
