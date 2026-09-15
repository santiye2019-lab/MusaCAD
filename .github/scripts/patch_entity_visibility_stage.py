from pathlib import Path


def patch(path,replacements):
    p=Path(path);s=p.read_text()
    for old,new,count in replacements:
        n=s.count(old)
        if n!=count: raise SystemExit(f'{path}: expected {count}, got {n}: {old[:180]!r}')
        s=s.replace(old,new,count)
    p.write_text(s)

patch('app/src/main/java/com/musa/cad/DxfBlocks.java',[
('''        if(++result.visits>500000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");\n        if(r.type.equals("SEQEND"))return;\n''','''        if(++result.visits>500000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");\n        if(r.type.equals("SEQEND"))return;\n        if((int)r.number(60,0)!=0)return; // Common entity visibility: 1 means invisible.\n''',1),
])

patch('app/src/main/java/com/musa/cad/DxfParser.java',[
('''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==62||code==66||code==67||code==68||code==69||\n''','''        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==60||code==62||code==66||code==67||code==68||code==69||\n''',1),
('''            if("VERTEX".equals(type)){\n                c.pending.points.add(new PointF((float)r.number(10,0),(float)r.number(20,0)));\n                c.pending.bulges.add(r.number(42,0));return;\n            }\n''','''            if("VERTEX".equals(type)){\n                if(r.integer(60,0)==0){c.pending.points.add(new PointF((float)r.number(10,0),(float)r.number(20,0)));c.pending.bulges.add(r.number(42,0));}\n                return;\n            }\n''',1),
('''        if("POLYLINE".equals(type)){\n            c.pending=new PendingPoly(target,r.text(8,"0"),r.text(5,""),(((int)r.number(70,0))&1)!=0,\n''','''        if(r.integer(60,0)!=0)return; // Honor the common DXF entity invisibility flag in streaming mode too.\n        if("POLYLINE".equals(type)){\n            c.pending=new PendingPoly(target,r.text(8,"0"),r.text(5,""),(((int)r.number(70,0))&1)!=0,\n''',1),
])

p=Path('tests/DxfBlocksTest.java');s=p.read_text()
old='''        DxfBlocks.Result repeated=read(b,insert("A")+insert("A",10,100));\n        if(repeated.placements.size()!=2||repeated.skipped!=0)throw new AssertionError("Repeated block");\n        skipped(read(b,insert("A",70,1001)));\n\n'''
new='''        DxfBlocks.Result repeated=read(b,insert("A")+insert("A",10,100));\n        if(repeated.placements.size()!=2||repeated.skipped!=0)throw new AssertionError("Repeated block");\n        skipped(read(b,insert("A",70,1001)));\n\n        DxfBlocks.Result hiddenRoot=read("",tags(0,"LINE",60,1,10,3,20,4,11,5,21,4));\n        if(!hiddenRoot.placements.isEmpty()||hiddenRoot.skipped!=0)throw new AssertionError("Invisible root entity");\n        DxfBlocks.Result hiddenInsert=read(b,insert("A",60,1));\n        if(!hiddenInsert.placements.isEmpty()||hiddenInsert.skipped!=0)throw new AssertionError("Invisible INSERT");\n        String hiddenMember=block("H",tags(0,"LINE",60,1,10,3,20,4,11,5,21,4)+LINE);\n        DxfBlocks.Result hiddenInside=read(hiddenMember,insert("H"));\n        if(hiddenInside.placements.size()!=1||hiddenInside.skipped!=0)throw new AssertionError("Invisible block member");\n\n'''
if s.count(old)!=1: raise SystemExit('DxfBlocksTest visibility marker mismatch')
s=s.replace(old,new,1).replace('System.out.println("22 block expansion and space/layout cases passed");','System.out.println("25 block expansion, visibility and space/layout cases passed");',1)
p.write_text(s)
