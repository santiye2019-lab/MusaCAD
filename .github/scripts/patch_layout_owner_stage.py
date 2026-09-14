from pathlib import Path

def patch(path,replacements):
    p=Path(path);s=p.read_text()
    for old,new,count in replacements:
        n=s.count(old)
        if n!=count: raise SystemExit(f'{path}: expected {count}, got {n}: {old[:180]!r}')
        s=s.replace(old,new,count)
    p.write_text(s)

patch('app/src/main/java/com/musa/cad/DxfBlocks.java',[
('''        Map<String,Block> blocks=new HashMap<>();List<Record> roots=new ArrayList<>();\n        LinkedHashSet<String> layoutNames=new LinkedHashSet<>();\n''','''        Map<String,Block> blocks=new HashMap<>();List<Record> roots=new ArrayList<>();\n        LinkedHashSet<String> layoutNames=new LinkedHashSet<>();Map<String,String> layoutByOwner=new HashMap<>();\n''',1),
('''            else if(section.equals("OBJECTS")&&record.type.equals("LAYOUT")){\n                String name=record.text(1,"").trim();if(!name.isEmpty())layoutNames.add(DxfSpace.normalizeName(name));\n            }\n''','''            else if(section.equals("OBJECTS")&&record.type.equals("LAYOUT")){\n                String name=DxfSpace.layoutObjectName(tags,record.from,record.to);String owner=DxfSpace.layoutObjectOwner(tags,record.from,record.to);\n                layoutNames.add(name);if(!owner.isEmpty())layoutByOwner.put(owner,name);\n            }\n''',1),
('''            String layout=sequenceLayout!=null?sequenceLayout:\n                DxfSpace.layout((int)root.number(67,0),root.text(410,""));\n''','''            String layout=sequenceLayout!=null?sequenceLayout:\n                DxfSpace.layout((int)root.number(67,0),root.text(410,""),root.text(330,""),layoutByOwner);\n''',1),
])

patch('app/src/main/java/com/musa/cad/DxfParser.java',[
('''        final Map<String,String> drawOrder=new HashMap<>();\n        final LinkedHashSet<String> layouts=new LinkedHashSet<>();\n''','''        final Map<String,String> drawOrder=new HashMap<>();\n        final Map<String,String> layoutByOwner=new HashMap<>();\n        final LinkedHashSet<String> layouts=new LinkedHashSet<>();\n''',1),
('''        finishPending(context);\n        context.layerTable.ensureDefaultLayer();\n''','''        finishPending(context);resolveStreamLayouts(context);\n        context.layerTable.ensureDefaultLayer();\n''',1),
('''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==62||code==66||code==67||code==68||code==69||\n            code==331||code==370||code==410||code==420||code==440||\n''','''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==62||code==66||code==67||code==68||code==69||\n            code==330||code==331||code==370||code==410||code==420||code==440||\n''',1),
('''        String layout=c.rootSequenceLayout!=null?c.rootSequenceLayout:DxfSpace.layout(r.integer(67,0),r.text(410,""));\n        c.layouts.add(layout);\n''','''        int paper=r.integer(67,0);String rawLayout=r.text(410,"");\n        String layout=c.rootSequenceLayout!=null?c.rootSequenceLayout:\n            (paper==1&&rawLayout.trim().isEmpty()?DxfSpace.unresolvedPaper(r.text(330,"")):DxfSpace.layout(paper,rawLayout));\n        if(!DxfSpace.isUnresolvedPaper(layout))c.layouts.add(layout);\n''',1),
('''        if("OBJECTS".equals(c.section)&&"LAYOUT".equals(type)){\n            String name=r.text(1,"").trim();if(!name.isEmpty())c.layouts.add(DxfSpace.normalizeName(name));return;\n        }\n''','''        if("OBJECTS".equals(c.section)&&"LAYOUT".equals(type)){\n            String name=DxfSpace.layoutObjectName(r.tags,0,r.tags.size());String owner=DxfSpace.layoutObjectOwner(r.tags,0,r.tags.size());\n            c.layouts.add(name);if(!owner.isEmpty())c.layoutByOwner.put(owner,name);return;\n        }\n''',1),
('''    private static void finishPending(StreamContext c)throws IOException{\n''','''    private static void resolveStreamLayouts(StreamContext c){\n        LinkedHashMap<String,ArrayList<StreamNode>> resolved=new LinkedHashMap<>();\n        for(Map.Entry<String,ArrayList<StreamNode>> entry:c.rootsByLayout.entrySet()){\n            String layout=DxfSpace.resolveUnresolved(entry.getKey(),c.layoutByOwner);c.layouts.add(layout);\n            resolved.computeIfAbsent(layout,k->new ArrayList<>()).addAll(entry.getValue());\n        }\n        c.rootsByLayout.clear();c.rootsByLayout.putAll(resolved);c.layouts.addAll(c.layoutByOwner.values());\n    }\n\n    private static void finishPending(StreamContext c)throws IOException{\n''',1),
])
