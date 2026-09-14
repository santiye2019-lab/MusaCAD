from pathlib import Path

p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

def rep(old,new):
    global s
    c=s.count(old)
    if c!=1:
        raise SystemExit(f'expected one match, got {c}: {old[:120]!r}')
    s=s.replace(old,new,1)

rep('''    private static final class Transformed implements Entity {
        final Entity entity;final Matrix matrix;
        Transformed(Entity entity,DxfBlocks.Transform t)throws IOException {
            this.entity=entity;
            float[] v={(float)t.a,(float)t.c,(float)t.x,(float)t.b,(float)t.d,(float)t.y,0,0,1};
            for(float n:v)if(!Float.isFinite(n))throw new IOException("DXF blok dönüşümü sınır dışında");
            matrix=new Matrix();matrix.setValues(v);
        }''','''    private static final class Transformed implements Entity {
        final Entity entity;final Matrix matrix;
        Transformed(Entity entity,DxfBlocks.Transform t)throws IOException {
            this(entity,new double[]{t.a,t.b,t.c,t.d,t.x,t.y});
        }
        Transformed(Entity entity,double[] t)throws IOException {
            this.entity=entity;
            if(t==null||t.length!=6)throw new IOException("Geçersiz DXF dönüşümü");
            float[] v={(float)t[0],(float)t[2],(float)t[4],(float)t[1],(float)t[3],(float)t[5],0,0,1};
            for(float n:v)if(!Float.isFinite(n))throw new IOException("DXF dönüşümü sınır dışında");
            matrix=new Matrix();matrix.setValues(v);
        }''')

rep('''            int color=DxfColor.argb(item.color,layerColors);''','''            entity=projectOcs(item.record.type,entity,lines,item.record.from,item.record.to);
            int color=DxfColor.argb(item.color,layerColors);''')

rep('''    private static final class PendingPoly {
        final ArrayList<StreamNode> target;final String layer,lineType;final boolean closed;final int aci,trueColor,lineWeight;final double lineTypeScale;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,boolean closed,int aci,int trueColor,String lineType,int lineWeight,double lineTypeScale){
            this.target=target;this.layer=layer;this.closed=closed;this.aci=aci;this.trueColor=trueColor;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }
    }''','''    private static final class PendingPoly {
        final ArrayList<StreamNode> target;final String layer,lineType;final boolean closed;final int aci,trueColor,lineWeight;final double lineTypeScale;
        final double[] ocs;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,boolean closed,int aci,int trueColor,String lineType,int lineWeight,double lineTypeScale,double[] ocs){
            this.target=target;this.layer=layer;this.closed=closed;this.aci=aci;this.trueColor=trueColor;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;this.ocs=ocs;
        }
    }''')

rep('''            c.pending=new PendingPoly(target,r.text(8,"0"),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1)));return;''','''            c.pending=new PendingPoly(target,r.text(8,"0"),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1)),
                ocsMatrix("POLYLINE",r.tags,0,r.tags.size()));return;''')

rep('''        Entity entity=parse(type,r.tags,0,r.tags.size());
        if(entity==null)c.skipped++;else target.add(new StreamShape(entity,r.text(8,"0"),
            r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1))));''','''        Entity entity=parse(type,r.tags,0,r.tags.size());
        if(entity==null)c.skipped++;else{
            entity=projectOcs(type,entity,r.tags,0,r.tags.size());
            target.add(new StreamShape(entity,r.text(8,"0"),r.integer(62,DxfColor.BYLAYER),r.trueColor(),
                r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1))));
        }''')

rep('''    private static void finishPending(StreamContext c){
        PendingPoly p=c.pending;if(p==null)return;c.pending=null;
        if(p.points.size()<2)c.skipped++;else p.target.add(new StreamShape(bulgedPoly(p.points,p.bulges,p.closed),p.layer,p.aci,p.trueColor,
            p.lineType,p.lineWeight,p.lineTypeScale));
    }''','''    private static void finishPending(StreamContext c)throws IOException{
        PendingPoly p=c.pending;if(p==null)return;c.pending=null;
        if(p.points.size()<2){c.skipped++;return;}
        Entity entity=bulgedPoly(p.points,p.bulges,p.closed);
        if(p.ocs!=null)entity=new Transformed(entity,p.ocs);
        p.target.add(new StreamShape(entity,p.layer,p.aci,p.trueColor,p.lineType,p.lineWeight,p.lineTypeScale));
    }''')

rep('''            Matrix transform=new Matrix();
            if(entity instanceof Transformed){transform=((Transformed)entity).matrix;entity=((Transformed)entity).entity;}''','''            Matrix transform=new Matrix();
            while(entity instanceof Transformed){
                Transformed wrappedTransform=(Transformed)entity;Matrix combined=new Matrix();
                combined.setConcat(transform,wrappedTransform.matrix);transform=combined;entity=wrappedTransform.entity;
            }''')

anchor='''    private static Entity parse(String type,List<String>a,int from,int to){'''
helpers='''    private static Entity projectOcs(String type,Entity entity,List<String>a,int from,int to)throws IOException{
        double[] matrix=ocsMatrix(type,a,from,to);
        return matrix==null?entity:new Transformed(entity,matrix);
    }

    /**
     * DXF uses OCS/ECS coordinates for selected planar entities. WCS-native entities
     * (LINE, POINT, MTEXT, ELLIPSE, SPLINE, LEADER and 3DFACE) must not be transformed here.
     */
    private static double[] ocsMatrix(String type,List<String>a,int from,int to){
        boolean supported="TEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type)||
            "CIRCLE".equals(type)||"ARC".equals(type)||"LWPOLYLINE".equals(type)||"POLYLINE".equals(type)||
            "HATCH".equals(type)||"SOLID".equals(type)||"TRACE".equals(type);
        if(!supported)return null;
        if("POLYLINE".equals(type)){
            int flags=(int)fv(a,from,to,70,0f);
            if((flags&(8|16|64))!=0)return null; // 3D/polyface/mesh coordinates are not planar OCS here.
        }
        double ex=fv(a,from,to,210,0f),ey=fv(a,from,to,220,0f),ez=fv(a,from,to,230,1f);
        if(Math.abs(ex)<1e-12&&Math.abs(ey)<1e-12&&ez>0)return null;
        double elevation="LWPOLYLINE".equals(type)?fv(a,from,to,38,0f):fv(a,from,to,30,0f);
        try{return DxfOcs.plane2d(ex,ey,ez,elevation);}
        catch(IllegalArgumentException invalid){return null;}
    }

'''
if s.count(anchor)!=1: raise SystemExit('parse anchor mismatch')
s=s.replace(anchor,helpers+anchor,1)

p.write_text(s)
