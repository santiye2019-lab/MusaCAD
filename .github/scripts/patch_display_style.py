from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

def rep(old,new):
    global s
    c=s.count(old)
    if c!=1:
        raise SystemExit(f'expected one match, got {c}: {old[:90]!r}')
    s=s.replace(old,new,1)

rep('    private static final int SIZE=2400,MARGIN=80;', '    private static final int SIZE=2400,MARGIN=80;\n    private static final int BACKGROUND=Color.rgb(18,24,30);')

rep('    private static final class Circle implements Entity{', '''    private static final class Wipeout implements Entity{
        final ArrayList<PointF> pts;
        Wipeout(ArrayList<PointF> pts){this.pts=pts;}
        public void bounds(RectF b){for(PointF q:pts)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(pts.size()<3)return;Path path=new Path();float[] v={pts.get(0).x,pts.get(0).y};m.mapPoints(v);path.moveTo(v[0],v[1]);
            for(int i=1;i<pts.size();i++){v[0]=pts.get(i).x;v[1]=pts.get(i).y;m.mapPoints(v);path.lineTo(v[0],v[1]);}path.close();
            Paint.Style oldStyle=p.getStyle();int oldColor=p.getColor();float oldWidth=p.getStrokeWidth();PathEffect oldEffect=p.getPathEffect();
            p.setStyle(Paint.Style.FILL);p.setColor(BACKGROUND);p.setPathEffect(null);c.drawPath(path,p);
            p.setStyle(oldStyle);p.setColor(oldColor);p.setStrokeWidth(oldWidth);p.setPathEffect(oldEffect);
        }
    }

    private static final class Circle implements Entity{''')

rep('''    /** Flattened entity with its effective AutoCAD color already resolved. */
    private static final class LayerEntity implements Entity {
        final Entity entity; final String layer; final int color;
        LayerEntity(Entity e,String l,int color){entity=e;layer=l;this.color=color;}
        public void bounds(RectF b){entity.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){
            p.setColor(color);
            entity.draw(c,p,m);
        }
    }''','''    /** Flattened entity with its effective AutoCAD display style resolved. */
    private static final class LayerEntity implements Entity {
        final Entity entity;final String layer;final int color,lineWeight;final DxfLineTypes.Pattern lineType;final double lineTypeScale;
        LayerEntity(Entity e,String l,int color,int lineWeight,DxfLineTypes.Pattern lineType,double lineTypeScale){
            entity=e;layer=l;this.color=color;this.lineWeight=lineWeight;this.lineType=lineType;this.lineTypeScale=lineTypeScale;
        }
        public void bounds(RectF b){entity.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){
            p.setColor(color);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(DxfStyle.strokeWidthPx(lineWeight));
            float[] vectors={1,0,0,1};m.mapVectors(vectors);
            double deviceScale=(Math.hypot(vectors[0],vectors[1])+Math.hypot(vectors[2],vectors[3]))*.5;
            float[] dash=DxfLineTypes.dashIntervals(lineType,lineTypeScale,deviceScale);
            p.setPathEffect(dash==null?null:new DashPathEffect(dash,0));
            entity.draw(c,p,m);p.setPathEffect(null);
        }
    }''')

rep('''        DxfLayerTable.Table layerTable=DxfLayerTable.parse(lines);
        Map<String,Integer> layerColors=layerTable.colors;
        DxfBlocks.Result expanded=DxfBlocks.expand(lines);''','''        DxfLayerTable.Table layerTable=DxfLayerTable.parse(lines);
        DxfLineTypes.Table lineTypes=DxfLineTypes.parse(lines);
        Map<String,Integer> layerColors=layerTable.colors;
        DxfBlocks.Result expanded=DxfBlocks.expand(lines);''')
rep('''            int color=DxfColor.argb(item.color,layerColors);
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer,color));''','''            int color=DxfColor.argb(item.color,layerColors);
            String effectiveType=DxfStyle.effectiveLineType(item.lineType,item.layer,layerTable.lineTypes);
            int effectiveWeight=DxfStyle.effectiveLineWeight(item.lineWeight,item.layer,layerTable.lineWeights);
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer,color,effectiveWeight,
                lineTypes.get(effectiveType),item.lineTypeScale*lineTypes.globalScale));''')

rep('''    private static final class StreamShape implements StreamNode {
        final Entity entity;final String layer;final int aci,trueColor;
        StreamShape(Entity entity,String layer,int aci,int trueColor){
            this.entity=entity;this.layer=layer;this.aci=aci;this.trueColor=trueColor;
        }
    }''','''    private static final class StreamShape implements StreamNode {
        final Entity entity;final String layer;final int aci,trueColor,lineWeight;final String lineType;final double lineTypeScale;
        StreamShape(Entity entity,String layer,int aci,int trueColor,String lineType,int lineWeight,double lineTypeScale){
            this.entity=entity;this.layer=layer;this.aci=aci;this.trueColor=trueColor;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }
    }''')
rep('''        final String name,layer;final int aci,trueColor;
        final double x,y,z,sx,sy,rotation,ex,ey,ez;''','''        final String name,layer,lineType;final int aci,trueColor,lineWeight;
        final double x,y,z,sx,sy,rotation,ex,ey,ez,lineTypeScale;''')
rep('''            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            x=r.number(10,0);''','''            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            lineType=r.text(6,DxfStyle.BYLAYER);lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));
            x=r.number(10,0);''')
rep('''    private static final class StreamDimension implements StreamNode {
        final String name,layer;final int aci,trueColor;
        StreamDimension(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
        }
    }''','''    private static final class StreamDimension implements StreamNode {
        final String name,layer,lineType;final int aci,trueColor,lineWeight;
        StreamDimension(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            lineType=r.text(6,DxfStyle.BYLAYER);lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);
        }
    }''')
rep('''        double number(int code,double fallback)throws IOException{
            String value=text(code,Double.toString(fallback));
            try{double n=Double.parseDouble(value);if(!Double.isFinite(n))throw new NumberFormatException();return n;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}
        }''','''        double number(int code,double fallback)throws IOException{
            String value=text(code,Double.toString(fallback));
            try{double n=Double.parseDouble(value);if(!Double.isFinite(n))throw new NumberFormatException();return n;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}
        }
        double numberAt(int tagIndex,double fallback)throws IOException{
            if(tagIndex<0||tagIndex+1>=tags.size())return fallback;
            try{double n=Double.parseDouble(tags.get(tagIndex+1).trim());if(!Double.isFinite(n))throw new NumberFormatException();return n;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}
        }''')
rep('''        final ArrayList<StreamNode> target;final String layer;final boolean closed;final int aci,trueColor;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,boolean closed,int aci,int trueColor){
            this.target=target;this.layer=layer;this.closed=closed;this.aci=aci;this.trueColor=trueColor;
        }''','''        final ArrayList<StreamNode> target;final String layer,lineType;final boolean closed;final int aci,trueColor,lineWeight;final double lineTypeScale;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,boolean closed,int aci,int trueColor,String lineType,int lineWeight,double lineTypeScale){
            this.target=target;this.layer=layer;this.closed=closed;this.aci=aci;this.trueColor=trueColor;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }''')
rep('''        final HashMap<String,StreamBlock> blocks=new HashMap<>();
        final DxfLayerTable.Table layerTable=new DxfLayerTable.Table();
        final ArrayList<StreamNode> roots=new ArrayList<>();''','''        final HashMap<String,StreamBlock> blocks=new HashMap<>();
        final DxfLayerTable.Table layerTable=new DxfLayerTable.Table();
        final DxfLineTypes.Table lineTypes=new DxfLineTypes.Table();
        final ArrayList<StreamNode> roots=new ArrayList<>();''')
rep('''        expandStream(context.roots,new DxfBlocks.Transform(),"0",null,context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context);''','''        expandStream(context.roots,new DxfBlocks.Transform(),"0",null,null,DxfStyle.LW_BYLAYER,
            context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context);''')
rep('''        return code==1||code==2||code==3||code==8||code==62||code==420||(code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;''','''        return code==1||code==2||code==3||code==6||code==8||code==9||code==62||code==370||code==420||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;''')
rep('''        if("SECTION".equals(type)){finishPending(c);c.section=r.text(2,"");c.activeBlock=null;return;}''','''        if("SECTION".equals(type)){
            finishPending(c);c.section=r.text(2,"");c.activeBlock=null;
            if("HEADER".equals(c.section)){
                for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==9&&"$LTSCALE".equalsIgnoreCase(r.tags.get(i+1).trim())){
                    for(int j=i+2;j+1<r.tags.size();j+=2){int code=intOf(r.tags.get(j));if(code==9)break;if(code==40){double v=r.numberAt(j,1);if(v>0)c.lineTypes.globalScale=v;break;}}
                }
            }
            return;
        }''')
rep('''        if("TABLES".equals(c.section)&&"LAYER".equals(type)){
            String layerName=r.text(2,"0");
            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor());return;
        }''','''        if("TABLES".equals(c.section)&&"LAYER".equals(type)){
            String layerName=r.text(2,"0");
            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor(),r.text(6,DxfStyle.CONTINUOUS),r.integer(370,DxfStyle.LW_DEFAULT));return;
        }
        if("TABLES".equals(c.section)&&"LTYPE".equals(type)){
            ArrayList<Double> values=new ArrayList<>();
            for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==49)values.add(r.numberAt(i,0));
            c.lineTypes.add(r.text(2,DxfStyle.CONTINUOUS),values);return;
        }''')
rep('''            c.pending=new PendingPoly(target,r.text(8,"0"),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor());return;''','''            c.pending=new PendingPoly(target,r.text(8,"0"),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1)));return;''')
rep('''        if(entity==null)c.skipped++;else target.add(new StreamShape(entity,r.text(8,"0"),
            r.integer(62,DxfColor.BYLAYER),r.trueColor()));''','''        if(entity==null)c.skipped++;else target.add(new StreamShape(entity,r.text(8,"0"),
            r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1))));''')
rep('''        if(p.points.size()<2)c.skipped++;else p.target.add(new StreamShape(bulgedPoly(p.points,p.bulges,p.closed),p.layer,p.aci,p.trueColor));''','''        if(p.points.size()<2)c.skipped++;else p.target.add(new StreamShape(bulgedPoly(p.points,p.bulges,p.closed),p.layer,p.aci,p.trueColor,
            p.lineType,p.lineWeight,p.lineTypeScale));''')
rep('''    private static void expandStream(List<StreamNode> nodes,DxfBlocks.Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                                     Map<String,StreamBlock> blocks,Set<String> stack,ArrayList<Entity> entities,Set<String> layers,''','''    private static void expandStream(List<StreamNode> nodes,DxfBlocks.Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                                     String byBlockLineType,int byBlockLineWeight,Map<String,StreamBlock> blocks,Set<String> stack,ArrayList<Entity> entities,Set<String> layers,''')
rep('''                int color=DxfColor.argb(ref,context.layerTable.colors);
                entities.add(new LayerEntity(new Transformed(shape.entity,parent),layer,color));''','''                int color=DxfColor.argb(ref,context.layerTable.colors);
                String semanticType=DxfStyle.resolveLineType(shape.lineType,byBlockLineType);
                int semanticWeight=DxfStyle.resolveLineWeight(shape.lineWeight,byBlockLineWeight);
                String effectiveType=DxfStyle.effectiveLineType(semanticType,layer,context.layerTable.lineTypes);
                int effectiveWeight=DxfStyle.effectiveLineWeight(semanticWeight,layer,context.layerTable.lineWeights);
                entities.add(new LayerEntity(new Transformed(shape.entity,parent),layer,color,effectiveWeight,
                    context.lineTypes.get(effectiveType),shape.lineTypeScale*context.lineTypes.globalScale));''')
rep('''                DxfColor.Ref dimColor=DxfColor.resolve(dimension.aci,dimension.trueColor,layer,byBlockColor);
                stack.add(dimension.name);
                expandStream(block.members,parent,layer,dimColor,blocks,stack,entities,layers,visibleLayers,counter,context);''','''                DxfColor.Ref dimColor=DxfColor.resolve(dimension.aci,dimension.trueColor,layer,byBlockColor);
                String dimType=DxfStyle.resolveLineType(dimension.lineType,byBlockLineType);
                int dimWeight=DxfStyle.resolveLineWeight(dimension.lineWeight,byBlockLineWeight);
                stack.add(dimension.name);
                expandStream(block.members,parent,layer,dimColor,dimType,dimWeight,blocks,stack,entities,layers,visibleLayers,counter,context);''')
rep('''            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            DxfBlocks.Transform local=DxfBlocks.Transform.insert(block.bx,block.by,insert.sx,insert.sy,insert.rotation,insert.x,insert.y);
            DxfBlocks.Transform transform=parent.thenLocal(local);stack.add(insert.name);
            expandStream(block.members,transform,layer,insertColor,blocks,stack,entities,layers,visibleLayers,counter,context);''','''            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            String insertType=DxfStyle.resolveLineType(insert.lineType,byBlockLineType);
            int insertWeight=DxfStyle.resolveLineWeight(insert.lineWeight,byBlockLineWeight);
            DxfBlocks.Transform local=DxfBlocks.Transform.insert(block.bx,block.by,insert.sx,insert.sy,insert.rotation,insert.x,insert.y);
            DxfBlocks.Transform transform=parent.thenLocal(local);stack.add(insert.name);
            expandStream(block.members,transform,layer,insertColor,insertType,insertWeight,blocks,stack,entities,layers,visibleLayers,counter,context);''')
rep('            Canvas canvas=new Canvas(bitmap);canvas.drawColor(Color.rgb(18,24,30));','            Canvas canvas=new Canvas(bitmap);canvas.drawColor(BACKGROUND);')
rep('        if("HATCH".equals(type))return parseHatch(a,from,to);','        if("HATCH".equals(type))return parseHatch(a,from,to);\n        if("WIPEOUT".equals(type))return parseWipeout(a,from,to);')
anchor='    private static Entity lwPolyline(List<String>a,int from,int to){'
helper='''    private static Entity parseWipeout(List<String>a,int from,int to){
        ArrayList<PointF> clip=repeatedPoints(a,from,to,14,24);ArrayList<double[]> raw=new ArrayList<>();
        for(PointF q:clip)raw.add(new double[]{q.x,q.y});
        double[] packed=DxfWipeout.boundary(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,13,1f),fv(a,from,to,23,1f),raw);
        ArrayList<PointF> points=packedPoints(packed);return points.size()<3?null:new Wipeout(points);
    }

'''
if s.count(anchor)!=1: raise SystemExit('wipeout anchor missing')
s=s.replace(anchor,helper+anchor,1)
p.write_text(s)
