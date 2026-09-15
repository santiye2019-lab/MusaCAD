from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java')
s=p.read_text()
old='''        RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);\n        for(Entity e:entities){FileTransfer.checkCancelled();e.bounds(b);}\n        if(!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top)return null;\n'''
new='''        RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);\n        boolean hasVisibleBounds=false;\n        for(Entity e:entities){\n            FileTransfer.checkCancelled();LayerEntity layer=(LayerEntity)e;if(!visibleLayers.contains(layer.layer))continue;\n            layer.bounds(b);hasVisibleBounds=true;\n        }\n        if(!hasVisibleBounds||!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top){\n            b.set(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);\n            for(Entity e:entities){FileTransfer.checkCancelled();e.bounds(b);}\n        }\n        if(!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top)return null;\n'''
if s.count(old)!=1: raise SystemExit(f'finish bounds marker mismatch: {s.count(old)}')
p.write_text(s.replace(old,new,1))
