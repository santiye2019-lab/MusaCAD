from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()

def repl(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:220]!r}')
    s=s.replace(old,new,count)

repl('''        if("HATCH".equals(type))return parseHatch(a,from,to);
        if("WIPEOUT".equals(type))return parseWipeout(a,from,to);
''','''        if("HATCH".equals(type))return parseHatch(a,from,to);
        if("WIPEOUT".equals(type))return parseWipeout(a,from,to);
        if("IMAGE".equals(type))return parseImageFrame(a,from,to);
''')

marker='''    private static Entity parseWipeout(List<String>a,int from,int to){
'''
method='''    /** External raster bytes are normally not embedded in DWG/DXF; keep the IMAGE footprint visible as a safe fallback. */
    private static Entity parseImageFrame(List<String>a,int from,int to){
        int display=(int)fv(a,from,to,70,1f);if((display&1)==0)return null;
        ArrayList<double[]> raw=new ArrayList<>();
        if((display&4)!=0){ArrayList<PointF> clip=repeatedPoints(a,from,to,14,24);for(PointF q:clip)raw.add(new double[]{q.x,q.y});}
        double[] packed=DxfWipeout.boundary(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,13,1f),fv(a,from,to,23,1f),raw);
        ArrayList<PointF> points=packedPoints(packed);return points.size()<3?null:new Poly(points,true);
    }

'''+marker
repl(marker,method)
p.write_text(s)
