from pathlib import Path

b=Path('app/src/main/java/com/musa/cad/DxfBlocks.java')
s=b.read_text()
old='''public final class DxfBlocks {\n'''
new='''public final class DxfBlocks {\n    /** Safety ceiling sized to accept the largest current real-world MusaCAD regression drawings. */\n    public static final int MAX_EXPANSION_VISITS=750000;\n'''
if s.count(old)!=1: raise SystemExit('DxfBlocks class marker mismatch')
s=s.replace(old,new,1)
old='''        if(++result.visits>500000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");\n'''
new='''        if(++result.visits>MAX_EXPANSION_VISITS)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");\n'''
if s.count(old)!=1: raise SystemExit('DxfBlocks visit guard mismatch')
s=s.replace(old,new,1);b.write_text(s)

p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()
old='''            if(++counter.visits>500000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");\n'''
new='''            if(++counter.visits>DxfBlocks.MAX_EXPANSION_VISITS)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");\n'''
if s.count(old)!=1: raise SystemExit('streaming visit guard mismatch')
s=s.replace(old,new,1);p.write_text(s)
