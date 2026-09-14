package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Expands 2D INSERTs, MINSERT arrays, nested blocks and DIMENSION graphics blocks. */
public final class DxfBlocks {
    public static final class Transform {
        public final double a,b,c,d,x,y;
        public Transform(){this(1,0,0,1,0,0);}
        private Transform(double a,double b,double c,double d,double x,double y){this.a=a;this.b=b;this.c=c;this.d=d;this.x=x;this.y=y;}
        public double[] point(double px,double py){return new double[]{a*px+c*py+x,b*px+d*py+y};}
        public Transform thenLocal(Transform t){return new Transform(a*t.a+c*t.b,b*t.a+d*t.b,a*t.c+c*t.d,b*t.c+d*t.d,a*t.x+c*t.y+x,b*t.x+d*t.y+y);}
        static Transform insert(double bx,double by,double sx,double sy,double degrees,double x,double y){
            return insert(bx,by,0,sx,sy,1,degrees,x,y,0,0,0,1,0,0);
        }
        static Transform insert(double bx,double by,double bz,double sx,double sy,double sz,double degrees,
                                double x,double y,double z,double ex,double ey,double ez,double arrayX,double arrayY){
            final double[] m;
            try{m=DxfOcs.insert2d(bx,by,bz,sx,sy,sz,degrees,x,y,z,ex,ey,ez,arrayX,arrayY);}
            catch(IllegalArgumentException e){throw e;}
            return new Transform(m[0],m[1],m[2],m[3],m[4],m[5]);
        }
    }

    public static final class Record {
        public final String type;
        public final int from,to;
        private final List<String> tags;
        Record(String type,List<String> tags,int from,int to){this.type=type;this.tags=tags;this.from=from;this.to=to;}
        String text(int code,String fallback){for(int i=from;i+1<to;i+=2)if(Integer.parseInt(tags.get(i).trim())==code)return tags.get(i+1).trim();return fallback;}
        int trueColor()throws IOException{
            String raw=text(420,"").trim();if(raw.isEmpty())return DxfColor.NO_TRUE_COLOR;
            try{return DxfColor.trueColor(Long.parseLong(raw));}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF TrueColor değeri",e);}
        }
        double number(int code,double fallback)throws IOException{
            try{double v=Double.parseDouble(text(code,Double.toString(fallback)));if(!Double.isFinite(v))throw new NumberFormatException();return v;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF blok koordinatı",e);}
        }
    }

    public static final class Placement {
        public final Record record;public final Transform transform;public final String layer;public final DxfColor.Ref color;
        public final String lineType;public final int lineWeight;public final double lineTypeScale;
        Placement(Record record,Transform transform,String layer,DxfColor.Ref color,String lineType,int lineWeight,double lineTypeScale){
            this.record=record;this.transform=transform;this.layer=layer;this.color=color;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }
    }

    private static final class Block {
        final Record header;final List<Record> members=new ArrayList<>();
        Block(Record header){this.header=header;}
    }

    public static final class Result {
        public final List<Placement> placements=new ArrayList<>();
        public int skipped;
        private int visits;
    }

    public static Result expand(List<String> tags)throws IOException{
        List<Record> records=new ArrayList<>();
        try{
            if(tags.size()%2!=0)throw new NumberFormatException();
            for(int i=0;i<tags.size();i+=2)Integer.parseInt(tags.get(i).trim());
            for(int i=0;i<tags.size();){
                if(Integer.parseInt(tags.get(i).trim())!=0){i+=2;continue;}
                String type=tags.get(i+1).trim();int from=i+2;i=from;
                while(i<tags.size()&&Integer.parseInt(tags.get(i).trim())!=0)i+=2;
                records.add(new Record(type,tags,from,i));
            }
        }catch(NumberFormatException e){throw new IOException("Geçersiz ASCII DXF etiketleri",e);}
        Map<String,Block> blocks=new HashMap<>();List<Record> roots=new ArrayList<>();
        String section="";Block active=null;
        for(Record record:records){
            if(record.type.equals("SECTION")){section=record.text(2,"");active=null;continue;}
            if(record.type.equals("ENDSEC")){section="";active=null;continue;}
            if(section.equals("BLOCKS")){
                if(record.type.equals("BLOCK")){active=new Block(record);blocks.put(key(record.text(2,"")),active);}
                else if(record.type.equals("ENDBLK"))active=null;
                else if(active!=null)active.members.add(record);
            }else if(section.equals("ENTITIES"))roots.add(record);
        }
        Result result=new Result();
        for(Record root:roots)expand(root,new Transform(),"0",null,null,DxfStyle.LW_BYLAYER,blocks,new HashSet<>(),result);
        return result;
    }

    private static void expand(Record r,Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                               String byBlockLineType,int byBlockLineWeight,
                               Map<String,Block> blocks,Set<String> stack,Result result)throws IOException{
        if(Thread.currentThread().isInterrupted())throw new java.io.InterruptedIOException("Yükleme iptal edildi");
        if(++result.visits>500000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");
        if(r.type.equals("SEQEND"))return;
        String layer=r.text(8,"0");if(layer.equals("0"))layer=parentLayer;
        DxfColor.Ref color=DxfColor.resolve((int)r.number(62,DxfColor.BYLAYER),r.trueColor(),layer,byBlockColor);
        String lineType=DxfStyle.resolveLineType(r.text(6,DxfStyle.BYLAYER),byBlockLineType);
        int lineWeight=DxfStyle.resolveLineWeight((int)r.number(370,DxfStyle.LW_BYLAYER),byBlockLineWeight);
        double lineTypeScale=DxfStyle.saneScale(r.number(48,1d));

        if(r.type.equals("DIMENSION")){
            String name=key(r.text(2,""));Block block=blocks.get(name);
            if(block==null||stack.contains(name)||stack.size()>=32||
                (((int)block.header.number(70,0))&12)!=0||!block.header.text(1,"").isEmpty()){
                result.skipped++;return;
            }
            stack.add(name);
            for(Record member:block.members)expand(member,parent,layer,color,lineType,lineWeight,blocks,stack,result);
            stack.remove(name);
            return;
        }

        if(!r.type.equals("INSERT")){
            result.placements.add(new Placement(r,parent,layer,color,lineType,lineWeight,lineTypeScale));return;
        }
        String name=key(r.text(2,""));Block block=blocks.get(name);
        if(block==null||stack.contains(name)||stack.size()>=32){result.skipped++;return;}
        if((((int)block.header.number(70,0))&12)!=0||!block.header.text(1,"").isEmpty()){
            result.skipped++;return;
        }
        double sx=r.number(41,1),sy=r.number(42,1),sz=r.number(43,1);
        if(sx==0||sy==0||sz==0){result.skipped++;return;}
        int columns=(int)r.number(70,1),rows=(int)r.number(71,1);
        if(columns<1||rows<1||columns>1000||rows>1000||(long)columns*rows>10000L){result.skipped++;return;}
        double columnSpacing=r.number(44,0),rowSpacing=r.number(45,0);
        double bx=block.header.number(10,0),by=block.header.number(20,0),bz=block.header.number(30,0);
        double x=r.number(10,0),y=r.number(20,0),z=r.number(30,0),rotation=r.number(50,0);
        double ex=r.number(210,0),ey=r.number(220,0),ez=r.number(230,1);
        stack.add(name);
        try{
            for(int row=0;row<rows;row++)for(int column=0;column<columns;column++){
                final Transform local;
                try{
                    local=Transform.insert(bx,by,bz,sx,sy,sz,rotation,x,y,z,ex,ey,ez,
                        column*columnSpacing,row*rowSpacing);
                }catch(IllegalArgumentException invalid){result.skipped++;continue;}
                Transform transform=parent.thenLocal(local);
                for(Record member:block.members)expand(member,transform,layer,color,lineType,lineWeight,blocks,stack,result);
            }
        }finally{stack.remove(name);}
    }

    private static String key(String s){return s.toUpperCase(Locale.ROOT);}
    private DxfBlocks(){}
}
