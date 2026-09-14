from pathlib import Path


def patch_file(path, transform):
    p=Path(path); s=p.read_text(); n=transform(s); p.write_text(n)


def replace_once(s, old, new, label):
    c=s.count(old)
    if c!=1: raise SystemExit(f'{label}: expected one match, got {c}')
    return s.replace(old,new,1)


def patch_layer(s):
    s=replace_once(s,
'''        public final Map<String,Integer> lineWeights=new HashMap<>();
        public final Set<String> names=new LinkedHashSet<>();''',
'''        public final Map<String,Integer> lineWeights=new HashMap<>();
        public final Map<String,Integer> opacities=new HashMap<>();
        public final Set<String> names=new LinkedHashSet<>();''','layer opacity map')
    s=replace_once(s,
'''        public void add(String rawName,int aci,int flags,int trueColor){
            add(rawName,aci,flags,trueColor,DxfStyle.CONTINUOUS,DxfStyle.LW_DEFAULT);
        }

        public void add(String rawName,int aci,int flags,int trueColor,String lineType,int lineWeight){
            String name=(rawName==null||rawName.trim().isEmpty())?"0":rawName.trim();''',
'''        public void add(String rawName,int aci,int flags,int trueColor){
            add(rawName,aci,flags,trueColor,DxfStyle.CONTINUOUS,DxfStyle.LW_DEFAULT,DxfTransparency.UNSET);
        }

        public void add(String rawName,int aci,int flags,int trueColor,String lineType,int lineWeight){
            add(rawName,aci,flags,trueColor,lineType,lineWeight,DxfTransparency.UNSET);
        }

        public void add(String rawName,int aci,int flags,int trueColor,String lineType,int lineWeight,long transparency){
            String name=(rawName==null||rawName.trim().isEmpty())?"0":rawName.trim();''','layer add overload')
    s=replace_once(s,
'''            lineWeights.put(key,lineWeight);
            visible.remove(canonical);''',
'''            lineWeights.put(key,lineWeight);
            opacities.put(key,DxfTransparency.layerOpacity(transparency));
            visible.remove(canonical);''','layer opacity store')
    s=replace_once(s,
'''                String lineType=text(tags,from,i,6,DxfStyle.CONTINUOUS);
                int lineWeight=integer(tags,from,i,370,DxfStyle.LW_DEFAULT);
                table.add(name,aci,flags,trueColor,lineType,lineWeight);''',
'''                String lineType=text(tags,from,i,6,DxfStyle.CONTINUOUS);
                int lineWeight=integer(tags,from,i,370,DxfStyle.LW_DEFAULT);
                long transparency=longInteger(tags,from,i,440,DxfTransparency.UNSET);
                table.add(name,aci,flags,trueColor,lineType,lineWeight,transparency);''','layer parse transparency')
    s=replace_once(s,
'''    private static int integer(List<String> tags,int from,int to,int wanted,int fallback){
        String value=text(tags,from,to,wanted,Integer.toString(fallback)).trim();
        return Integer.parseInt(value);
    }
    private DxfLayerTable(){}''',
'''    private static int integer(List<String> tags,int from,int to,int wanted,int fallback){
        String value=text(tags,from,to,wanted,Integer.toString(fallback)).trim();
        return Integer.parseInt(value);
    }
    private static long longInteger(List<String> tags,int from,int to,int wanted,long fallback){
        String value=text(tags,from,to,wanted,Long.toString(fallback)).trim();
        return Long.parseLong(value);
    }
    private DxfLayerTable(){}''','layer long helper')
    return s


def patch_blocks(s):
    s=replace_once(s,
'''        double number(int code,double fallback)throws IOException{
            try{double v=Double.parseDouble(text(code,Double.toString(fallback)));if(!Double.isFinite(v))throw new NumberFormatException();return v;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF blok koordinatı",e);}
        }
    }''',
'''        double number(int code,double fallback)throws IOException{
            try{double v=Double.parseDouble(text(code,Double.toString(fallback)));if(!Double.isFinite(v))throw new NumberFormatException();return v;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF blok koordinatı",e);}
        }
        long longInteger(int code,long fallback)throws IOException{
            try{return Long.parseLong(text(code,Long.toString(fallback)));}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}
        }
    }''','block long helper')
    s=replace_once(s,
'''        public final Record record;public final Transform transform;public final String layer;public final DxfColor.Ref color;
        public final String lineType;public final int lineWeight;public final double lineTypeScale;
        Placement(Record record,Transform transform,String layer,DxfColor.Ref color,String lineType,int lineWeight,double lineTypeScale){
            this.record=record;this.transform=transform;this.layer=layer;this.color=color;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }''',
'''        public final Record record;public final Transform transform;public final String layer;public final DxfColor.Ref color;
        public final DxfTransparency.Ref transparency;
        public final String lineType;public final int lineWeight;public final double lineTypeScale;
        Placement(Record record,Transform transform,String layer,DxfColor.Ref color,DxfTransparency.Ref transparency,String lineType,int lineWeight,double lineTypeScale){
            this.record=record;this.transform=transform;this.layer=layer;this.color=color;this.transparency=transparency;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }''','placement transparency')
    s=replace_once(s,
'''        Result result=new Result();
        for(Record root:roots)expand(root,new Transform(),"0",null,null,DxfStyle.LW_BYLAYER,blocks,new HashSet<>(),result);''',
'''        Map<String,String> drawOrder=DxfDrawOrder.parse(tags);
        if(!drawOrder.isEmpty())roots.sort((a,b)->DxfDrawOrder.compare(a.text(5,""),b.text(5,""),drawOrder));
        Result result=new Result();
        for(Record root:roots)expand(root,new Transform(),"0",null,null,null,DxfStyle.LW_BYLAYER,blocks,new HashSet<>(),result);''','block root draw order')
    s=replace_once(s,
'''    private static void expand(Record r,Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                               String byBlockLineType,int byBlockLineWeight,
                               Map<String,Block> blocks,Set<String> stack,Result result)throws IOException{''',
'''    private static void expand(Record r,Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                               DxfTransparency.Ref byBlockTransparency,String byBlockLineType,int byBlockLineWeight,
                               Map<String,Block> blocks,Set<String> stack,Result result)throws IOException{''','block expand signature')
    s=replace_once(s,
'''        DxfColor.Ref color=DxfColor.resolve((int)r.number(62,DxfColor.BYLAYER),r.trueColor(),layer,byBlockColor);
        String lineType=DxfStyle.resolveLineType(r.text(6,DxfStyle.BYLAYER),byBlockLineType);''',
'''        DxfColor.Ref color=DxfColor.resolve((int)r.number(62,DxfColor.BYLAYER),r.trueColor(),layer,byBlockColor);
        DxfTransparency.Ref transparency=DxfTransparency.resolve(r.longInteger(440,DxfTransparency.UNSET),layer,byBlockTransparency);
        String lineType=DxfStyle.resolveLineType(r.text(6,DxfStyle.BYLAYER),byBlockLineType);''','block transparency resolve')
    s=s.replace('expand(member,parent,layer,color,lineType,lineWeight,blocks,stack,result)',
                'expand(member,parent,layer,color,transparency,lineType,lineWeight,blocks,stack,result)')
    s=replace_once(s,
'''            result.placements.add(new Placement(r,parent,layer,color,lineType,lineWeight,lineTypeScale));return;''',
'''            result.placements.add(new Placement(r,parent,layer,color,transparency,lineType,lineWeight,lineTypeScale));return;''','placement create')
    s=s.replace('expand(member,transform,layer,color,lineType,lineWeight,blocks,stack,result)',
                'expand(member,transform,layer,color,transparency,lineType,lineWeight,blocks,stack,result)')
    return s


def patch_parser(s):
    poly_anchor='''    private static final class Wipeout implements Entity{'''
    filled='''    private static final class FilledPoly implements Entity{
        final ArrayList<PointF> pts;
        FilledPoly(ArrayList<PointF> pts){this.pts=pts;}
        public void bounds(RectF b){for(PointF q:pts)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(pts.size()<3)return;Path path=new Path();float[] v={pts.get(0).x,pts.get(0).y};m.mapPoints(v);path.moveTo(v[0],v[1]);
            for(int i=1;i<pts.size();i++){v[0]=pts.get(i).x;v[1]=pts.get(i).y;m.mapPoints(v);path.lineTo(v[0],v[1]);}path.close();
            Paint.Style old=p.getStyle();PathEffect effect=p.getPathEffect();p.setPathEffect(null);p.setStyle(Paint.Style.FILL);c.drawPath(path,p);
            p.setStyle(old);p.setPathEffect(effect);
        }
    }

'''
    if poly_anchor not in s: raise SystemExit('FilledPoly anchor missing')
    if 'private static final class FilledPoly' not in s:s=s.replace(poly_anchor,filled+poly_anchor,1)
    s=replace_once(s,
'''            int color=DxfColor.argb(item.color,layerColors);''',
'''            int color=DxfTransparency.apply(DxfColor.argb(item.color,layerColors),DxfTransparency.opacity(item.transparency,layerTable.opacities));''','buffered transparency')
    s=replace_once(s,
'''    private static final class StreamShape implements StreamNode {
        final Entity entity;final String layer;final int aci,trueColor,lineWeight;final String lineType;final double lineTypeScale;
        StreamShape(Entity entity,String layer,int aci,int trueColor,String lineType,int lineWeight,double lineTypeScale){
            this.entity=entity;this.layer=layer;this.aci=aci;this.trueColor=trueColor;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }
    }''',
'''    private static final class StreamShape implements StreamNode {
        final Entity entity;final String layer,handle;final int aci,trueColor,lineWeight;final String lineType;final double lineTypeScale;final long transparencyRaw;
        StreamShape(Entity entity,String layer,String handle,int aci,int trueColor,long transparencyRaw,String lineType,int lineWeight,double lineTypeScale){
            this.entity=entity;this.layer=layer;this.handle=handle;this.aci=aci;this.trueColor=trueColor;this.transparencyRaw=transparencyRaw;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }
    }''','stream shape')
    s=replace_once(s,
'''    private static final class StreamInsert implements StreamNode {
        final String name,layer,lineType;final int aci,trueColor,lineWeight;
        final double x,y,z,sx,sy,sz,rotation,ex,ey,ez,lineTypeScale,columnSpacing,rowSpacing;
        final int columns,rows;
        StreamInsert(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();''',
'''    private static final class StreamInsert implements StreamNode {
        final String name,layer,lineType,handle;final int aci,trueColor,lineWeight;final long transparencyRaw;
        final double x,y,z,sx,sy,sz,rotation,ex,ey,ez,lineTypeScale,columnSpacing,rowSpacing;
        final int columns,rows;
        StreamInsert(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");handle=r.text(5,"");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();transparencyRaw=r.longInteger(440,DxfTransparency.UNSET);''','stream insert')
    s=replace_once(s,
'''    private static final class StreamDimension implements StreamNode {
        final String name,layer,lineType;final int aci,trueColor,lineWeight;
        StreamDimension(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();''',
'''    private static final class StreamDimension implements StreamNode {
        final String name,layer,lineType,handle;final int aci,trueColor,lineWeight;final long transparencyRaw;
        StreamDimension(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");handle=r.text(5,"");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();transparencyRaw=r.longInteger(440,DxfTransparency.UNSET);''','stream dimension')
    s=replace_once(s,
'''        int integer(int code,int fallback)throws IOException{
            String value=text(code,Integer.toString(fallback));
            try{return Integer.parseInt(value);}catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}
        }
        int trueColor()throws IOException{''',
'''        int integer(int code,int fallback)throws IOException{
            String value=text(code,Integer.toString(fallback));
            try{return Integer.parseInt(value);}catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}
        }
        long longInteger(int code,long fallback)throws IOException{
            String value=text(code,Long.toString(fallback));
            try{return Long.parseLong(value);}catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}
        }
        int trueColor()throws IOException{''','stream long helper')
    s=replace_once(s,
'''    private static final class PendingPoly {
        final ArrayList<StreamNode> target;final String layer,lineType;final boolean closed;final int aci,trueColor,lineWeight;final double lineTypeScale;
        final double[] ocs;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,boolean closed,int aci,int trueColor,String lineType,int lineWeight,double lineTypeScale,double[] ocs){
            this.target=target;this.layer=layer;this.closed=closed;this.aci=aci;this.trueColor=trueColor;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;this.ocs=ocs;
        }
    }''',
'''    private static final class PendingPoly {
        final ArrayList<StreamNode> target;final String layer,lineType,handle;final boolean closed;final int aci,trueColor,lineWeight;final double lineTypeScale;final long transparencyRaw;
        final double[] ocs;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,String handle,boolean closed,int aci,int trueColor,long transparencyRaw,String lineType,int lineWeight,double lineTypeScale,double[] ocs){
            this.target=target;this.layer=layer;this.handle=handle;this.closed=closed;this.aci=aci;this.trueColor=trueColor;this.transparencyRaw=transparencyRaw;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;this.ocs=ocs;
        }
    }''','pending poly')
    s=replace_once(s,
'''        final DxfTextStyles.Table textStyles=new DxfTextStyles.Table();
        final ArrayList<StreamNode> roots=new ArrayList<>();''',
'''        final DxfTextStyles.Table textStyles=new DxfTextStyles.Table();
        final Map<String,String> drawOrder=new HashMap<>();
        final ArrayList<StreamNode> roots=new ArrayList<>();''','stream draw order map')
    s=replace_once(s,
'''        StreamCounter counter=new StreamCounter();
        expandStream(context.roots,new DxfBlocks.Transform(),"0",null,null,DxfStyle.LW_BYLAYER,
            context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context);''',
'''        if(!context.drawOrder.isEmpty())context.roots.sort((a,b)->DxfDrawOrder.compare(streamHandle(a),streamHandle(b),context.drawOrder));
        StreamCounter counter=new StreamCounter();
        expandStream(context.roots,new DxfBlocks.Transform(),"0",null,null,null,DxfStyle.LW_BYLAYER,
            context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context);''','stream sort/expand')
    s=replace_once(s,
'''        return code==1||code==2||code==3||code==4||code==6||code==7||code==8||code==9||code==62||code==370||code==420||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;''',
'''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==62||code==331||code==370||code==420||code==440||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;''','stream keep codes')
    s=replace_once(s,
'''        if("TABLES".equals(c.section)&&"LAYER".equals(type)){
            String layerName=r.text(2,"0");
            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor(),r.text(6,DxfStyle.CONTINUOUS),r.integer(370,DxfStyle.LW_DEFAULT));return;
        }''',
'''        if("TABLES".equals(c.section)&&"LAYER".equals(type)){
            String layerName=r.text(2,"0");
            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor(),r.text(6,DxfStyle.CONTINUOUS),r.integer(370,DxfStyle.LW_DEFAULT),
                r.longInteger(440,DxfTransparency.UNSET));return;
        }''','stream layer transparency')
    marker='''        ArrayList<StreamNode> target=streamTarget(c);if(target==null)return;'''
    s=replace_once(s,marker,
'''        if("OBJECTS".equals(c.section)&&"SORTENTSTABLE".equals(type)){
            DxfDrawOrder.addRecord(r.tags,0,r.tags.size(),c.drawOrder);return;
        }
        ArrayList<StreamNode> target=streamTarget(c);if(target==null)return;''','stream sortents')
    s=replace_once(s,
'''            c.pending=new PendingPoly(target,r.text(8,"0"),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1)),
                ocsMatrix("POLYLINE",r.tags,0,r.tags.size()));return;''',
'''            c.pending=new PendingPoly(target,r.text(8,"0"),r.text(5,""),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.longInteger(440,DxfTransparency.UNSET),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1)),
                ocsMatrix("POLYLINE",r.tags,0,r.tags.size()));return;''','stream poly metadata')
    s=replace_once(s,
'''            target.add(new StreamShape(entity,r.text(8,"0"),r.integer(62,DxfColor.BYLAYER),r.trueColor(),
                r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1))));''',
'''            target.add(new StreamShape(entity,r.text(8,"0"),r.text(5,""),r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.longInteger(440,DxfTransparency.UNSET),
                r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1))));''','stream generic metadata')
    s=replace_once(s,
'''        p.target.add(new StreamShape(entity,p.layer,p.aci,p.trueColor,p.lineType,p.lineWeight,p.lineTypeScale));''',
'''        p.target.add(new StreamShape(entity,p.layer,p.handle,p.aci,p.trueColor,p.transparencyRaw,p.lineType,p.lineWeight,p.lineTypeScale));''','pending shape metadata')
    s=replace_once(s,
'''    private static void expandStream(List<StreamNode> nodes,DxfBlocks.Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                                     String byBlockLineType,int byBlockLineWeight,Map<String,StreamBlock> blocks,Set<String> stack,ArrayList<Entity> entities,Set<String> layers,''',
'''    private static void expandStream(List<StreamNode> nodes,DxfBlocks.Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                                     DxfTransparency.Ref byBlockTransparency,String byBlockLineType,int byBlockLineWeight,Map<String,StreamBlock> blocks,Set<String> stack,ArrayList<Entity> entities,Set<String> layers,''','stream expand signature')
    s=replace_once(s,
'''                DxfColor.Ref ref=DxfColor.resolve(shape.aci,shape.trueColor,layer,byBlockColor);
                int color=DxfColor.argb(ref,context.layerTable.colors);''',
'''                DxfColor.Ref ref=DxfColor.resolve(shape.aci,shape.trueColor,layer,byBlockColor);
                DxfTransparency.Ref transparency=DxfTransparency.resolve(shape.transparencyRaw,layer,byBlockTransparency);
                int color=DxfTransparency.apply(DxfColor.argb(ref,context.layerTable.colors),DxfTransparency.opacity(transparency,context.layerTable.opacities));''','stream shape transparency')
    s=replace_once(s,
'''                DxfColor.Ref dimColor=DxfColor.resolve(dimension.aci,dimension.trueColor,layer,byBlockColor);
                String dimType=DxfStyle.resolveLineType(dimension.lineType,byBlockLineType);''',
'''                DxfColor.Ref dimColor=DxfColor.resolve(dimension.aci,dimension.trueColor,layer,byBlockColor);
                DxfTransparency.Ref dimTransparency=DxfTransparency.resolve(dimension.transparencyRaw,layer,byBlockTransparency);
                String dimType=DxfStyle.resolveLineType(dimension.lineType,byBlockLineType);''','stream dimension transparency')
    s=s.replace('expandStream(block.members,parent,layer,dimColor,dimType,dimWeight,blocks,stack,entities,layers,visibleLayers,counter,context)',
                'expandStream(block.members,parent,layer,dimColor,dimTransparency,dimType,dimWeight,blocks,stack,entities,layers,visibleLayers,counter,context)')
    s=replace_once(s,
'''            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            String insertType=DxfStyle.resolveLineType(insert.lineType,byBlockLineType);''',
'''            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            DxfTransparency.Ref insertTransparency=DxfTransparency.resolve(insert.transparencyRaw,layer,byBlockTransparency);
            String insertType=DxfStyle.resolveLineType(insert.lineType,byBlockLineType);''','stream insert transparency')
    s=s.replace('expandStream(block.members,transform,layer,insertColor,insertType,insertWeight,blocks,stack,entities,layers,visibleLayers,counter,context)',
                'expandStream(block.members,transform,layer,insertColor,insertTransparency,insertType,insertWeight,blocks,stack,entities,layers,visibleLayers,counter,context)')
    s=replace_once(s,
'''    private static String key(String value){return value.toUpperCase(Locale.ROOT);}''',
'''    private static String streamHandle(StreamNode node){
        if(node instanceof StreamShape)return ((StreamShape)node).handle;
        if(node instanceof StreamInsert)return ((StreamInsert)node).handle;
        if(node instanceof StreamDimension)return ((StreamDimension)node).handle;
        return "";
    }

    private static String key(String value){return value.toUpperCase(Locale.ROOT);}''','stream handle helper')
    s=replace_once(s,
'''        if("SOLID".equals(type)||"TRACE".equals(type)||"3DFACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<2?null:new Poly(p,true);
        }''',
'''        if("SOLID".equals(type)||"TRACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<3?null:new FilledPoly(p);
        }
        if("3DFACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<2?null:new Poly(p,true);
        }''','solid fill')
    return s


def patch_layer_test(s):
    s=replace_once(s,'"0","LAYER","2","Visible","70","0","62","3",',
                      '"0","LAYER","2","Visible","70","0","62","3","440","33554560",','layer test transparency data')
    s=replace_once(s,
'''        if(t.colors.get("VISIBLE")!=0xFF00FF00)throw new AssertionError("layer color");
        System.out.println("DXF layer table states passed");''',
'''        if(t.colors.get("VISIBLE")!=0xFF00FF00)throw new AssertionError("layer color");
        if(t.opacities.get("VISIBLE")!=128)throw new AssertionError("layer transparency");
        System.out.println("DXF layer table states passed");''','layer test assertion')
    return s


def patch_blocks_test(s):
    s=replace_once(s,'import com.musa.cad.DxfBlocks;','import com.musa.cad.DxfBlocks;\nimport com.musa.cad.DxfTransparency;','blocks test import')
    anchor='''        if(!layer.placements.get(0).layer.equals("BORU"))throw new AssertionError("Layer inheritance");
        skipped(read(b,insert("MISSING")));'''
    repl='''        if(!layer.placements.get(0).layer.equals("BORU"))throw new AssertionError("Layer inheritance");
        String transparentBlock=block("T",tags(0,"LINE",440,16777216,10,3,20,4,11,5,21,4));
        DxfBlocks.Result transparency=read(transparentBlock,insert("T",440,33554560));
        if(DxfTransparency.opacity(transparency.placements.get(0).transparency,Collections.emptyMap())!=128)throw new AssertionError("Transparency inheritance");
        skipped(read(b,insert("MISSING")));'''
    return replace_once(s,anchor,repl,'blocks transparency test')


def patch_ci(s):
    s=replace_once(s,
'''          javac -d /tmp/musacad-tests app/src/main/java/com/musa/cad/DxfColor.java app/src/main/java/com/musa/cad/DxfStyle.java app/src/main/java/com/musa/cad/DxfLayerTable.java tests/DxfLayerTableTest.java''',
'''          javac -d /tmp/musacad-tests app/src/main/java/com/musa/cad/DxfColor.java app/src/main/java/com/musa/cad/DxfStyle.java app/src/main/java/com/musa/cad/DxfTransparency.java app/src/main/java/com/musa/cad/DxfLayerTable.java tests/DxfLayerTableTest.java''','ci layer compile')
    s=replace_once(s,
'''      - name: Test DXF block expansion
        run: |
          javac -d /tmp/musacad-tests app/src/main/java/com/musa/cad/DxfColor.java app/src/main/java/com/musa/cad/DxfStyle.java app/src/main/java/com/musa/cad/DxfOcs.java app/src/main/java/com/musa/cad/DxfBlocks.java tests/DxfBlocksTest.java
          java -cp /tmp/musacad-tests DxfBlocksTest''',
'''      - name: Test DXF transparency and draw order
        run: |
          javac -d /tmp/musacad-tests app/src/main/java/com/musa/cad/DxfColor.java app/src/main/java/com/musa/cad/DxfTransparency.java app/src/main/java/com/musa/cad/DxfDrawOrder.java tests/DxfTransparencyTest.java tests/DxfDrawOrderTest.java
          java -cp /tmp/musacad-tests DxfTransparencyTest
          java -cp /tmp/musacad-tests DxfDrawOrderTest
      - name: Test DXF block expansion
        run: |
          javac -d /tmp/musacad-tests app/src/main/java/com/musa/cad/DxfColor.java app/src/main/java/com/musa/cad/DxfStyle.java app/src/main/java/com/musa/cad/DxfTransparency.java app/src/main/java/com/musa/cad/DxfDrawOrder.java app/src/main/java/com/musa/cad/DxfOcs.java app/src/main/java/com/musa/cad/DxfBlocks.java tests/DxfBlocksTest.java
          java -cp /tmp/musacad-tests DxfBlocksTest''','ci transparency draworder')
    return s

patch_file('app/src/main/java/com/musa/cad/DxfLayerTable.java',patch_layer)
patch_file('app/src/main/java/com/musa/cad/DxfBlocks.java',patch_blocks)
patch_file('app/src/main/java/com/musa/cad/DxfParser.java',patch_parser)
patch_file('tests/DxfLayerTableTest.java',patch_layer_test)
patch_file('tests/DxfBlocksTest.java',patch_blocks_test)
patch_file('.github/workflows/android.yml',patch_ci)
