from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

def rep(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:180]!r}')
    s=s.replace(old,new,count)

rep('''    private static void appendViewportEntities(ArrayList<Entity> target,List<LayerEntity> model,DxfViewport.Spec spec)throws IOException{\n        if((long)target.size()+model.size()>500000L)throw new IOException("VIEWPORT açıldığında nesne sınırı aşıldı");\n        for(LayerEntity item:model){FileTransfer.checkCancelled();target.add(item.inViewport(spec));}\n    }\n''','''    private static Set<String> viewportFrozenLayers(List<String>a,int from,int to,DxfLayerTable.Table layerTable){\n        HashSet<String> result=new HashSet<>();\n        for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==331){\n            String name=layerTable.nameForHandle(a.get(i+1));if(name!=null)result.add(DxfColor.key(name));\n        }\n        return result;\n    }\n\n    private static Set<String> viewportFrozenLayers(Collection<String> handles,DxfLayerTable.Table layerTable){\n        HashSet<String> result=new HashSet<>();if(handles==null)return result;\n        for(String handle:handles){String name=layerTable.nameForHandle(handle);if(name!=null)result.add(DxfColor.key(name));}\n        return result;\n    }\n\n    private static void appendViewportEntities(ArrayList<Entity> target,List<LayerEntity> model,DxfViewport.Spec spec,Set<String> frozenLayers)throws IOException{\n        if((long)target.size()+model.size()>500000L)throw new IOException("VIEWPORT açıldığında nesne sınırı aşıldı");\n        for(LayerEntity item:model){\n            FileTransfer.checkCancelled();if(frozenLayers!=null&&frozenLayers.contains(DxfColor.key(item.layer)))continue;target.add(item.inViewport(spec));\n        }\n    }\n''')

rep('''                if(viewportModel==null||!spec.supported()){skipped++;continue;}\n                appendViewportEntities(entities,viewportModel,spec);entity=viewportFrame(spec);\n''','''                if(viewportModel==null||!spec.supported()){skipped++;continue;}\n                Set<String> frozen=viewportFrozenLayers(lines,item.record.from,item.record.to,layerTable);\n                appendViewportEntities(entities,viewportModel,spec,frozen);entity=viewportFrame(spec);\n''')

rep('''    private static final class StreamViewport implements StreamNode {\n        final DxfViewport.Spec spec;final String layer,lineType,handle;final int aci,trueColor,lineWeight;final long transparencyRaw;final double lineTypeScale;\n        StreamViewport(StreamRecord r)throws IOException{\n''','''    private static final class StreamViewport implements StreamNode {\n        final DxfViewport.Spec spec;final String layer,lineType,handle;final int aci,trueColor,lineWeight;final long transparencyRaw;final double lineTypeScale;\n        final Set<String> frozenHandles=new LinkedHashSet<>();\n        StreamViewport(StreamRecord r)throws IOException{\n''')

rep('''            lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));\n        }\n    }\n\n    private static final class StreamInsert''','''            lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));\n            for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==331)frozenHandles.add(r.tags.get(i+1).trim());\n        }\n    }\n\n    private static final class StreamInsert''')

rep('''            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor(),r.text(6,DxfStyle.CONTINUOUS),r.integer(370,DxfStyle.LW_DEFAULT),\n                r.longInteger(440,DxfTransparency.UNSET));return;\n''','''            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor(),r.text(6,DxfStyle.CONTINUOUS),r.integer(370,DxfStyle.LW_DEFAULT),\n                r.longInteger(440,DxfTransparency.UNSET),r.text(5,""));return;\n''')

rep('''                if(viewportModel==null||!viewport.spec.supported()){context.skipped++;continue;}\n                appendViewportEntities(entities,viewportModel,viewport.spec);\n''','''                if(viewportModel==null||!viewport.spec.supported()){context.skipped++;continue;}\n                Set<String> frozen=viewportFrozenLayers(viewport.frozenHandles,context.layerTable);\n                appendViewportEntities(entities,viewportModel,viewport.spec,frozen);\n''')

p.write_text(s)
