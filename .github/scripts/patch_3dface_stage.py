from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()
old='''        if("3DFACE".equals(type)){\n            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<2?null:new Poly(p,true);\n        }\n'''
new='''        if("3DFACE".equals(type)){\n            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);\n            if(p.size()<3)return null;\n            return segmentSet(DxfFace.visibleEdges(pointArray(p),(int)fv(a,from,to,70,0f)));\n        }\n'''
if s.count(old)!=1: raise SystemExit(f'3DFACE marker count={s.count(old)}')
p.write_text(s.replace(old,new,1))
