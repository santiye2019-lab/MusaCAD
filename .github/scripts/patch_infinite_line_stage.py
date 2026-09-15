from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()

def repl(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:220]!r}')
    s=s.replace(old,new,count)

marker='''    private static final class SegmentSet implements Entity{\n'''
entity='''    private static final class InfiniteEntity implements Entity{\n        final float x,y,dx,dy;final boolean ray;\n        InfiniteEntity(float x,float y,float dx,float dy,boolean ray){this.x=x;this.y=y;this.dx=dx;this.dy=dy;this.ray=ray;}\n        public void bounds(RectF b){\n            double len=Math.hypot(dx,dy);if(len<1e-12){add(b,x,y);return;}float ux=(float)(dx/len),uy=(float)(dy/len);\n            add(b,x,y);add(b,x+ux,y+uy);if(!ray)add(b,x-ux,y-uy);\n        }\n        public void draw(Canvas c,Paint p,Matrix m){\n            float[] v={x,y,x+dx,y+dy};m.mapPoints(v);double[] q=DxfInfiniteLine.clip(v[0],v[1],v[2]-v[0],v[3]-v[1],ray,0,0,c.getWidth(),c.getHeight());\n            if(q.length==4)c.drawLine((float)q[0],(float)q[1],(float)q[2],(float)q[3],p);\n        }\n    }\n\n'''+marker
repl(marker,entity)

repl('''            if(entity instanceof Line){\n                Line line=(Line)entity;candidates=DxfSnapGeometry.line(line.x1,line.y1,line.x2,line.y2);\n            }else if(entity instanceof LeaderEntity){''','''            if(entity instanceof Line){\n                Line line=(Line)entity;candidates=DxfSnapGeometry.line(line.x1,line.y1,line.x2,line.y2);\n            }else if(entity instanceof InfiniteEntity){\n                InfiniteEntity infinite=(InfiniteEntity)entity;candidates=new double[]{infinite.x,infinite.y};\n            }else if(entity instanceof LeaderEntity){''')

repl('''        if("LINE".equals(type))return new Line(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21));\n        if("POINT".equals(type))''','''        if("LINE".equals(type))return new Line(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21));\n        if("RAY".equals(type)||"XLINE".equals(type)){\n            float dx=f(a,from,to,11),dy=f(a,from,to,21);if(Math.hypot(dx,dy)<1e-12)return null;\n            return new InfiniteEntity(f(a,from,to,10),f(a,from,to,20),dx,dy,"RAY".equals(type));\n        }\n        if("POINT".equals(type))''')

p.write_text(s)
