from pathlib import Path

def patch(path,replacements):
    p=Path(path);s=p.read_text()
    for old,new,count in replacements:
        n=s.count(old)
        if n!=count: raise SystemExit(f'{path}: expected {count}, got {n}: {old[:180]!r}')
        s=s.replace(old,new,count)
    p.write_text(s)

patch('app/src/main/java/com/musa/cad/DxfBlocks.java',[
('''        if(r.type.equals("SEQEND"))return;\n        if((int)r.number(60,0)!=0)return; // Common entity visibility: 1 means invisible.\n''','''        if(r.type.equals("SEQEND"))return;\n        if(DxfVisibility.invisible(r.type,(int)r.number(60,0),(int)r.number(70,0)))return;\n''',1),
])

patch('app/src/main/java/com/musa/cad/DxfParser.java',[
('''            if("VERTEX".equals(type)){\n                if(r.integer(60,0)==0){c.pending.points.add(new PointF((float)r.number(10,0),(float)r.number(20,0)));c.pending.bulges.add(r.number(42,0));}\n                return;\n            }\n''','''            if("VERTEX".equals(type)){\n                if(!DxfVisibility.invisible(type,r.integer(60,0),r.integer(70,0))){c.pending.points.add(new PointF((float)r.number(10,0),(float)r.number(20,0)));c.pending.bulges.add(r.number(42,0));}\n                return;\n            }\n''',1),
('''        if(r.integer(60,0)!=0)return; // Honor the common DXF entity invisibility flag in streaming mode too.\n''','''        if(DxfVisibility.invisible(type,r.integer(60,0),r.integer(70,0)))return;\n''',1),
])

p=Path('tests/DxfBlocksTest.java');s=p.read_text()
old='''        String hiddenMember=block("H",tags(0,"LINE",60,1,10,3,20,4,11,5,21,4)+LINE);\n        DxfBlocks.Result hiddenInside=read(hiddenMember,insert("H"));\n        if(hiddenInside.placements.size()!=1||hiddenInside.skipped!=0)throw new AssertionError("Invisible block member");\n\n'''
new='''        String hiddenMember=block("H",tags(0,"LINE",60,1,10,3,20,4,11,5,21,4)+LINE);\n        DxfBlocks.Result hiddenInside=read(hiddenMember,insert("H"));\n        if(hiddenInside.placements.size()!=1||hiddenInside.skipped!=0)throw new AssertionError("Invisible block member");\n        DxfBlocks.Result hiddenAttrib=read("",tags(0,"ATTRIB",70,1,1,"SECRET",10,3,20,4,40,1));\n        if(!hiddenAttrib.placements.isEmpty()||hiddenAttrib.skipped!=0)throw new AssertionError("Invisible ATTRIB flag");\n        DxfBlocks.Result visibleAttrib=read("",tags(0,"ATTRIB",70,2,1,"VISIBLE",10,3,20,4,40,1));\n        if(visibleAttrib.placements.size()!=1||visibleAttrib.skipped!=0)throw new AssertionError("Visible ATTRIB flags");\n        String hiddenAttdefBlock=block("AH",tags(0,"ATTDEF",70,1,1,"SECRET",10,3,20,4,40,1)+LINE);\n        DxfBlocks.Result hiddenAttdef=read(hiddenAttdefBlock,insert("AH"));\n        if(hiddenAttdef.placements.size()!=1||hiddenAttdef.skipped!=0)throw new AssertionError("Invisible ATTDEF in block");\n\n'''
if s.count(old)!=1: raise SystemExit('visibility tests marker mismatch')
s=s.replace(old,new,1).replace('System.out.println("25 block expansion, visibility and space/layout cases passed");','System.out.println("28 block expansion, visibility and space/layout cases passed");',1)
p.write_text(s)
