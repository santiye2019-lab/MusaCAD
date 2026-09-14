from pathlib import Path

p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()

def rep(old,new,count=1):
    global s
    actual=s.count(old)
    if actual!=count:
        raise SystemExit(f'expected {count} match(es), got {actual}: {old[:180]!r}')
    s=s.replace(old,new,count)

rep('''        public final Set<String> layerNames,visibleLayers;
        private final List<Entity> document;
        private final Matrix view;

        Result(Bitmap b,int e,int skipped,float[] points,List<Entity> document,Matrix view,Set<String> all,Set<String> visible){
            bitmap=b;entityCount=e;skippedCount=skipped;snapPoints=points;
            this.document=document;this.view=new Matrix(view);
            layerNames=Collections.unmodifiableSet(new TreeSet<>(all));
            visibleLayers=Collections.unmodifiableSet(new TreeSet<>(visible));layerCount=all.size();
        }

        public Result withVisibleLayers(Set<String> selected)throws IOException{
            Set<String> visible=new HashSet<>(selected);visible.retainAll(layerNames);
            Result result=renderLayers(document,view,layerNames,visible,skippedCount);
''','''        public final Set<String> layerNames,visibleLayers,layoutNames;
        public final String activeLayout;
        private final List<Entity> document;
        private final Matrix view;

        Result(Bitmap b,int e,int skipped,float[] points,List<Entity> document,Matrix view,Set<String> all,Set<String> visible,
               Set<String> layouts,String activeLayout){
            bitmap=b;entityCount=e;skippedCount=skipped;snapPoints=points;
            this.document=document;this.view=new Matrix(view);
            layerNames=Collections.unmodifiableSet(new TreeSet<>(all));
            visibleLayers=Collections.unmodifiableSet(new TreeSet<>(visible));layerCount=all.size();
            LinkedHashSet<String> names=new LinkedHashSet<>();if(layouts!=null)names.addAll(layouts);if(names.isEmpty())names.add(DxfSpace.MODEL);
            layoutNames=Collections.unmodifiableSet(names);this.activeLayout=DxfSpace.normalizeName(activeLayout);
        }

        public Result withVisibleLayers(Set<String> selected)throws IOException{
            Set<String> visible=new HashSet<>(selected);visible.retainAll(layerNames);
            Result result=renderLayers(document,view,layerNames,visible,skippedCount,layoutNames,activeLayout);
''')

rep('''        return finishEntities(entities,layers,visibleLayers,skipped);
    }

    private interface StreamNode {}''','''        return finishEntities(entities,layers,visibleLayers,skipped,expanded.layouts,expanded.activeLayout);
    }

    private interface StreamNode {}''')

rep('''    private static final class StreamContext {
        String section="";StreamBlock activeBlock;PendingPoly pending;int skipped;
        final HashMap<String,StreamBlock> blocks=new HashMap<>();
        final DxfLayerTable.Table layerTable=new DxfLayerTable.Table();
        final DxfLineTypes.Table lineTypes=new DxfLineTypes.Table();
        final DxfTextStyles.Table textStyles=new DxfTextStyles.Table();
        final Map<String,String> drawOrder=new HashMap<>();
        final ArrayList<StreamNode> roots=new ArrayList<>();
    }''','''    private static final class StreamContext {
        String section="",rootSequenceLayout;StreamBlock activeBlock;PendingPoly pending;int skipped;
        final HashMap<String,StreamBlock> blocks=new HashMap<>();
        final DxfLayerTable.Table layerTable=new DxfLayerTable.Table();
        final DxfLineTypes.Table lineTypes=new DxfLineTypes.Table();
        final DxfTextStyles.Table textStyles=new DxfTextStyles.Table();
        final Map<String,String> drawOrder=new HashMap<>();
        final LinkedHashSet<String> layouts=new LinkedHashSet<>();
        final LinkedHashMap<String,ArrayList<StreamNode>> rootsByLayout=new LinkedHashMap<>();
    }''')

rep('''        ArrayList<Entity> entities=new ArrayList<>();
        Set<String> layers=new HashSet<>(context.layerTable.names);
        Set<String> visibleLayers=new HashSet<>(context.layerTable.visible);
        if(!context.drawOrder.isEmpty())context.roots.sort((a,b)->DxfDrawOrder.compare(streamHandle(a),streamHandle(b),context.drawOrder));
        StreamCounter counter=new StreamCounter();
        expandStream(context.roots,new DxfBlocks.Transform(),"0",null,null,null,DxfStyle.LW_BYLAYER,
            context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context);
        return finishEntities(entities,layers,visibleLayers,context.skipped);
''','''        ArrayList<Entity> entities=new ArrayList<>();
        Set<String> layers=new HashSet<>(context.layerTable.names);
        Set<String> visibleLayers=new HashSet<>(context.layerTable.visible);
        if(context.layouts.isEmpty())context.layouts.add(DxfSpace.MODEL);
        String activeLayout=DxfSpace.chooseActive(context.rootsByLayout);
        ArrayList<StreamNode> roots=context.rootsByLayout.get(activeLayout);if(roots==null)roots=new ArrayList<>();
        if(!context.drawOrder.isEmpty())roots.sort((a,b)->DxfDrawOrder.compare(streamHandle(a),streamHandle(b),context.drawOrder));
        StreamCounter counter=new StreamCounter();
        expandStream(roots,new DxfBlocks.Transform(),"0",null,null,null,DxfStyle.LW_BYLAYER,
            context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context);
        return finishEntities(entities,layers,visibleLayers,context.skipped,context.layouts,activeLayout);
''')

rep('''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==62||code==331||code==370||code==420||code==440||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;
    }

    private static ArrayList<StreamNode> streamTarget(StreamContext c){
        if("ENTITIES".equals(c.section))return c.roots;
        if("BLOCKS".equals(c.section)&&c.activeBlock!=null)return c.activeBlock.members;
        return null;
    }
''','''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==62||code==66||code==67||
            code==331||code==370||code==410||code==420||code==440||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;
    }

    private static boolean streamSequenceMember(String type){
        return "VERTEX".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type)||"SEQEND".equals(type);
    }

    private static ArrayList<StreamNode> streamTarget(StreamContext c,String type,StreamRecord r)throws IOException{
        if("BLOCKS".equals(c.section)&&c.activeBlock!=null)return c.activeBlock.members;
        if(!"ENTITIES".equals(c.section))return null;
        boolean member=streamSequenceMember(type);
        if(c.rootSequenceLayout!=null&&!member)c.rootSequenceLayout=null;
        String layout=c.rootSequenceLayout!=null?c.rootSequenceLayout:DxfSpace.layout(r.integer(67,0),r.text(410,""));
        c.layouts.add(layout);
        ArrayList<StreamNode> target=c.rootsByLayout.computeIfAbsent(layout,k->new ArrayList<>());
        if("POLYLINE".equals(type)||("INSERT".equals(type)&&r.integer(66,0)!=0))c.rootSequenceLayout=layout;
        return target;
    }
''')

rep('''        if("OBJECTS".equals(c.section)&&"SORTENTSTABLE".equals(type)){
            DxfDrawOrder.addRecord(r.tags,0,r.tags.size(),c.drawOrder);return;
        }
        ArrayList<StreamNode> target=streamTarget(c);if(target==null)return;
''','''        if("OBJECTS".equals(c.section)&&"LAYOUT".equals(type)){
            String name=r.text(1,"").trim();if(!name.isEmpty())c.layouts.add(DxfSpace.normalizeName(name));return;
        }
        if("OBJECTS".equals(c.section)&&"SORTENTSTABLE".equals(type)){
            DxfDrawOrder.addRecord(r.tags,0,r.tags.size(),c.drawOrder);return;
        }
        ArrayList<StreamNode> target=streamTarget(c,type,r);if(target==null)return;
''')

rep('''            if("SEQEND".equals(type)){finishPending(c);return;}
            finishPending(c);
        }
''','''            if("SEQEND".equals(type)){finishPending(c);c.rootSequenceLayout=null;return;}
            finishPending(c);
        }
''')

rep('''        if("VERTEX".equals(type)||"SEQEND".equals(type))return;
        if("INSERT".equals(type)){target.add(new StreamInsert(r));return;}
''','''        if("VERTEX".equals(type))return;
        if("SEQEND".equals(type)){c.rootSequenceLayout=null;return;}
        if("INSERT".equals(type)){target.add(new StreamInsert(r));return;}
''')

rep('''    private static Result finishEntities(ArrayList<Entity> entities,Set<String> layers,Set<String> visibleLayers,int skipped)throws IOException{
        if(entities.isEmpty())return null;
''','''    private static Result finishEntities(ArrayList<Entity> entities,Set<String> layers,Set<String> visibleLayers,int skipped,
                                         Set<String> layoutNames,String activeLayout)throws IOException{
        if(entities.isEmpty())return null;
''')

rep('''        return renderLayers(entities,m,layers,visibleLayers,skipped);
    }

    private static Result renderLayers(List<Entity> document,Matrix view,Set<String> all,Set<String> visible,int skipped)throws IOException{
''','''        return renderLayers(entities,m,layers,visibleLayers,skipped,layoutNames,activeLayout);
    }

    private static Result renderLayers(List<Entity> document,Matrix view,Set<String> all,Set<String> visible,int skipped,
                                       Set<String> layoutNames,String activeLayout)throws IOException{
''')

rep('''            return new Result(bitmap,shown.size(),skipped,snapPoints(shown,view),document,view,all,visible);
''','''            return new Result(bitmap,shown.size(),skipped,snapPoints(shown,view),document,view,all,visible,layoutNames,activeLayout);
''')

p.write_text(s)
