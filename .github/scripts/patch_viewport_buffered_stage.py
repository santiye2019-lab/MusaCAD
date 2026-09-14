from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

marker='''    public static final class Result {\n'''
if s.count(marker)!=1: raise SystemExit('Result marker mismatch')
viewport_class='''    private static final class ViewportClip implements Entity {\n        final Entity entity;final Matrix modelToPaper;final DxfViewport.Spec spec;\n        ViewportClip(Entity entity,DxfViewport.Spec spec)throws IOException{\n            this.entity=entity;this.spec=spec;double[] t=spec.matrix();\n            float[] v={(float)t[0],(float)t[2],(float)t[4],(float)t[1],(float)t[3],(float)t[5],0,0,1};\n            for(float n:v)if(!Float.isFinite(n))throw new IOException("VIEWPORT dönüşümü sınır dışında");\n            modelToPaper=new Matrix();modelToPaper.setValues(v);\n        }\n        public void bounds(RectF b){add(b,(float)spec.left(),(float)spec.bottom());add(b,(float)spec.right(),(float)spec.top());}\n        public void draw(Canvas c,Paint p,Matrix view){\n            float[] xy={(float)spec.left(),(float)spec.bottom(),(float)spec.right(),(float)spec.bottom(),\n                (float)spec.right(),(float)spec.top(),(float)spec.left(),(float)spec.top()};view.mapPoints(xy);\n            Path clip=new Path();clip.moveTo(xy[0],xy[1]);clip.lineTo(xy[2],xy[3]);clip.lineTo(xy[4],xy[5]);clip.lineTo(xy[6],xy[7]);clip.close();\n            int save=c.save();c.clipPath(clip);Matrix combined=new Matrix();combined.setConcat(view,modelToPaper);entity.draw(c,p,combined);c.restoreToCount(save);\n        }\n    }\n\n'''
s=s.replace(marker,viewport_class+marker,1)

old='''        public void bounds(RectF b){entity.bounds(b);}\n        public void draw(Canvas c,Paint p,Matrix m){\n'''
new='''        public void bounds(RectF b){entity.bounds(b);}\n        LayerEntity inViewport(DxfViewport.Spec spec)throws IOException{\n            return new LayerEntity(new ViewportClip(entity,spec),layer,color,lineWeight,lineType,lineTypeScale);\n        }\n        public void draw(Canvas c,Paint p,Matrix m){\n'''
if s.count(old)!=1: raise SystemExit('LayerEntity marker mismatch')
s=s.replace(old,new,1)

start=s.index('    private static Result renderBuffered(File file,String preferredLayout)throws IOException{')
end=s.index('\n    private interface StreamNode {}',start)
new_block=r'''    private static DxfViewport.Spec viewportSpec(List<String>a,int from,int to){
        return DxfViewport.of(f(a,from,to,10),f(a,from,to,20),fv(a,from,to,40,0f),fv(a,from,to,41,0f),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,45,0f),fv(a,from,to,51,0f),
            (int)fv(a,from,to,69,0f),(int)fv(a,from,to,68,1f),
            fv(a,from,to,16,0f),fv(a,from,to,26,0f),fv(a,from,to,36,1f));
    }

    private static Entity viewportFrame(DxfViewport.Spec spec){
        ArrayList<PointF> p=new ArrayList<>();p.add(new PointF((float)spec.left(),(float)spec.bottom()));
        p.add(new PointF((float)spec.right(),(float)spec.bottom()));p.add(new PointF((float)spec.right(),(float)spec.top()));
        p.add(new PointF((float)spec.left(),(float)spec.top()));return new Poly(p,true);
    }

    private static void appendViewportEntities(ArrayList<Entity> target,List<LayerEntity> model,DxfViewport.Spec spec)throws IOException{
        if((long)target.size()+model.size()>500000L)throw new IOException("VIEWPORT açıldığında nesne sınırı aşıldı");
        for(LayerEntity item:model){FileTransfer.checkCancelled();target.add(item.inViewport(spec));}
    }

    private static int appendBufferedPlacements(List<String> lines,List<DxfBlocks.Placement> placements,
                                                 DxfLayerTable.Table layerTable,DxfLineTypes.Table lineTypes,DxfTextStyles.Table textStyles,
                                                 ArrayList<Entity> entities,Set<String> layers,Set<String> visibleLayers,
                                                 List<LayerEntity> viewportModel)throws IOException{
        int skipped=0;Map<String,Integer> layerColors=layerTable.colors;
        for(int index=0;index<placements.size();index++){
            FileTransfer.checkCancelled();DxfBlocks.Placement item=placements.get(index);Entity entity;
            if("VIEWPORT".equals(item.record.type)){
                DxfViewport.Spec spec=viewportSpec(lines,item.record.from,item.record.to);
                if(viewportModel==null||!spec.supported()){skipped++;continue;}
                appendViewportEntities(entities,viewportModel,spec);entity=viewportFrame(spec);
            }else if("POLYLINE".equals(item.record.type)){
                ArrayList<PointF> points=new ArrayList<>();ArrayList<Double> bulges=new ArrayList<>();int j=index+1;
                while(j<placements.size()&&"VERTEX".equals(placements.get(j).record.type)){
                    DxfBlocks.Record vertex=placements.get(j).record;
                    points.add(new PointF(f(lines,vertex.from,vertex.to,10),f(lines,vertex.from,vertex.to,20)));
                    bulges.add(vertex.number(42,0));j++;
                }
                if(points.size()<2){skipped++;continue;}
                entity=bulgedPoly(points,bulges,(((int)item.record.number(70,0))&1)!=0);index=j-1;
            }else if("VERTEX".equals(item.record.type)){
                skipped++;continue;
            }else{
                entity=isTextType(item.record.type)?parseTextEntity(item.record.type,lines,item.record.from,item.record.to,textStyles):
                    parse(item.record.type,lines,item.record.from,item.record.to);
                if(entity==null){skipped++;continue;}
            }
            entity=projectOcs(item.record.type,entity,lines,item.record.from,item.record.to);
            int color=DxfTransparency.apply(DxfColor.argb(item.color,layerColors),DxfTransparency.opacity(item.transparency,layerTable.opacities));
            String effectiveType=DxfStyle.effectiveLineType(item.lineType,item.layer,layerTable.lineTypes);
            int effectiveWeight=DxfStyle.effectiveLineWeight(item.lineWeight,item.layer,layerTable.lineWeights);
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer,color,effectiveWeight,
                lineTypes.get(effectiveType),item.lineTypeScale*lineTypes.globalScale));
            if(layers.add(item.layer)&&!layerTable.contains(item.layer))visibleLayers.add(item.layer);
        }
        return skipped;
    }

    private static Result renderBuffered(File file,String preferredLayout)throws IOException{
        List<String> lines=readLines(file);
        DxfLayerTable.Table layerTable=DxfLayerTable.parse(lines);
        DxfLineTypes.Table lineTypes=DxfLineTypes.parse(lines);
        DxfTextStyles.Table textStyles=DxfTextStyles.parse(lines);
        DxfBlocks.Result expanded=DxfBlocks.expand(lines,preferredLayout);
        ArrayList<Entity> entities=new ArrayList<>();Set<String> layers=new HashSet<>(layerTable.names);
        Set<String> visibleLayers=new HashSet<>(layerTable.visible);List<LayerEntity> viewportModel=null;
        if(!DxfSpace.isModel(expanded.activeLayout)){
            DxfBlocks.Result modelExpanded=DxfBlocks.expand(lines,DxfSpace.MODEL);ArrayList<Entity> modelRaw=new ArrayList<>();
            appendBufferedPlacements(lines,modelExpanded.placements,layerTable,lineTypes,textStyles,modelRaw,layers,visibleLayers,null);
            viewportModel=new ArrayList<>();for(Entity e:modelRaw)if(e instanceof LayerEntity)viewportModel.add((LayerEntity)e);
        }
        int skipped=expanded.skipped+appendBufferedPlacements(lines,expanded.placements,layerTable,lineTypes,textStyles,
            entities,layers,visibleLayers,viewportModel);
        return finishEntities(entities,layers,visibleLayers,skipped,expanded.layouts,expanded.activeLayout);
    }
'''
s=s[:start]+new_block+s[end:]
p.write_text(s)
