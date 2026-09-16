from pathlib import Path
p=Path('app/src/main/java/com/musa/cad/DxfParser.java');s=p.read_text()

def repl(old,new,count=1):
    global s
    n=s.count(old)
    if n!=count: raise SystemExit(f'expected {count}, got {n}: {old[:240]!r}')
    s=s.replace(old,new,count)

repl('''        if("IMAGE".equals(type))return parseImageFrame(a,from,to);\n        if("SOLID".equals(type)||"TRACE".equals(type)){\n''','''        if("IMAGE".equals(type))return parseImageFrame(a,from,to);\n        if("MESH".equals(type))return parseMesh(a,from,to);\n        if("SOLID".equals(type)||"TRACE".equals(type)){\n''')

marker='''    /** External raster bytes are normally not embedded in DWG/DXF; keep the IMAGE footprint visible as a safe fallback. */\n    private static Entity parseImageFrame(List<String>a,int from,int to){\n'''
insert='''    /** Modern AcDbSubDMesh rendered as a 2D WCS wireframe; subdivision surfaces are not synthesized. */\n    private static Entity parseMesh(List<String>a,int from,int to){\n        int vertexTag=-1,vertexCount=-1,faceTag=-1,faceItems=0;\n        for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));\n            if(code==92&&faceTag<0){vertexTag=i;vertexCount=(int)floatOf(a.get(i+1));}\n            else if(code==93){faceTag=i;faceItems=(int)floatOf(a.get(i+1));break;}\n        }\n        if(vertexTag<0||faceTag<0||vertexCount<2||faceItems<0)return null;\n        ArrayList<PointF> vertices=new ArrayList<>();Float vx=null;\n        for(int i=vertexTag+2;i+1<faceTag&&vertices.size()<vertexCount;i+=2){int code=intOf(a.get(i));\n            if(code==10)vx=floatOf(a.get(i+1));else if(code==20&&vx!=null){float vy=floatOf(a.get(i+1));if(Float.isFinite(vx)&&Float.isFinite(vy))vertices.add(new PointF(vx,vy));vx=null;}\n        }\n        if(vertices.size()<2)return null;double[] xy=pointArray(vertices);ArrayList<Integer> faces=new ArrayList<>();int edgeTag=-1,edgeCount=0;\n        for(int i=faceTag+2;i+1<to;i+=2){int code=intOf(a.get(i));\n            if(code==94){edgeTag=i;edgeCount=(int)floatOf(a.get(i+1));break;}\n            if(code==90&&faces.size()<faceItems)faces.add((int)floatOf(a.get(i+1)));\n        }\n        int[] faceData=new int[faces.size()];for(int i=0;i<faceData.length;i++)faceData[i]=faces.get(i);\n        ArrayList<Integer> edges=new ArrayList<>();if(edgeTag>=0&&edgeCount>0){\n            int wanted=Math.min(200000,edgeCount*2);for(int i=edgeTag+2;i+1<to&&edges.size()<wanted;i+=2){int code=intOf(a.get(i));if(code==95)break;if(code==90)edges.add((int)floatOf(a.get(i+1)));}\n        }\n        int[] edgeData=new int[edges.size()];for(int i=0;i<edgeData.length;i++)edgeData[i]=edges.get(i);\n        return segmentSet(DxfMesh.wireframe(xy,faceData,edgeData));\n    }\n\n'''+marker
repl(marker,insert)
p.write_text(s)
