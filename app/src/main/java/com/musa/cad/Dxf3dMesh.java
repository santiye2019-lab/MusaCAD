package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.HashSet;
import java.util.List;

/**
 * Bounded XYZ geometry reader for MusaCAD 3D.
 * Expands ordinary INSERT blocks and supports:
 * 3DFACE, polyface/polygon mesh, LINE, CIRCLE/ARC sampled outlines,
 * ordinary/3D POLYLINE, TRACE and SOLID geometry.
 */
public final class Dxf3dMesh {
    private static final int MAX_VERTICES=2_000_000,MAX_TRIANGLES=4_000_000,MAX_EDGES=4_000_000;
    public final float[] xyz;
    public final int[] triangles,edges,sourceLines,coordinateSlots;
    public final float cx,cy,cz,radius,minZ,maxZ;
    public final int acisSolids,unsupportedSolids,skippedBlockReferences,approximateCurves;
    public final boolean flatPlan,solidProxy;

    private Dxf3dMesh(float[] xyz,int[] triangles,int[] explicitEdges,BitSet outlineOnlyTriangles,int[] sourceLines,int[] coordinateSlots,int acisSolids,int unsupportedSolids,int skippedBlockReferences,int approximateCurves,boolean solidProxy){
        this.xyz=xyz;this.triangles=triangles;this.sourceLines=sourceLines;this.coordinateSlots=coordinateSlots;this.acisSolids=acisSolids;this.unsupportedSolids=unsupportedSolids;this.skippedBlockReferences=skippedBlockReferences;this.approximateCurves=approximateCurves;this.solidProxy=solidProxy;
        HashSet<Long> seen=new HashSet<>();IntBuffer lines=new IntBuffer();
        for(int i=0;i+1<explicitEdges.length;i+=2)edge(seen,lines,explicitEdges[i],explicitEdges[i+1]);
        for(int i=0;i+2<triangles.length;i+=3){
            // DXF 3DFACE/TRACE/SOLID have an outline independent of the
            // triangulation used for shaded drawing and area measurements.
            // Never draw a synthetic quad diagonal or a hidden 3DFACE edge.
            if(outlineOnlyTriangles.get(i/3))continue;
            edge(seen,lines,triangles[i],triangles[i+1]);
            edge(seen,lines,triangles[i+1],triangles[i+2]);
            edge(seen,lines,triangles[i+2],triangles[i]);
        }
        this.edges=lines.toArray();
        float minX=Float.MAX_VALUE,minY=Float.MAX_VALUE,minZ=Float.MAX_VALUE,maxX=-Float.MAX_VALUE,maxY=-Float.MAX_VALUE,maxZ=-Float.MAX_VALUE;
        for(int i=0;i<xyz.length;i+=3){
            minX=Math.min(minX,xyz[i]);maxX=Math.max(maxX,xyz[i]);
            minY=Math.min(minY,xyz[i+1]);maxY=Math.max(maxY,xyz[i+1]);
            minZ=Math.min(minZ,xyz[i+2]);maxZ=Math.max(maxZ,xyz[i+2]);
        }
        this.minZ=minZ;this.maxZ=maxZ;
        cx=(minX+maxX)*.5f;cy=(minY+maxY)*.5f;cz=(minZ+maxZ)*.5f;
        radius=Math.max(1e-6f,(float)Math.sqrt((maxX-minX)*(maxX-minX)+(maxY-minY)*(maxY-minY)+(maxZ-minZ)*(maxZ-minZ))*.5f);
        flatPlan=Math.abs(maxZ-minZ)<=Math.max(1e-5f,radius*1e-6f);
    }

    public static Dxf3dMesh read(File file)throws IOException{
        Collector collector=new Collector();
        DxfBlocks.Result expansion=DxfBlocks.expand3d(file,StandardCharsets.UTF_8,collector);
        collector.finishGeometry();
        if(collector.polyface)throw new IOException("3B polyface nesnesinin sonu bulunamadı");
        if(collector.solids>0&&(collector.points.n==0||(collector.triangles.n==0&&collector.explicitEdges.n==0))){
            Extents ext=readHeaderExtents(file);
            if(ext!=null){
                addExtentProxy(collector,ext);
                collector.solidProxy=true;
            }
        }
        if(collector.points.n==0||(collector.triangles.n==0&&collector.explicitEdges.n==0))
            throw new IOException(collector.solids>0
                ?collector.solids+" adet 3DSOLID/BODY/REGION bulundu; bu dosyada güvenilir sınır kutusu veya yüzey verisi çıkarılamadı"
                :"Bu çizimde desteklenen 3B veya çizgisel geometri bulunamadı");
        return new Dxf3dMesh(
            collector.points.toArray(),collector.triangles.toArray(),collector.explicitEdges.toArray(),
            collector.outlineOnlyTriangles,collector.sourceLines.toArray(),collector.coordinateSlots.toArray(),collector.solids,Math.max(0,collector.solids-collector.resolvedSolids),expansion.skipped,collector.approximateCurves,collector.solidProxy
        );
    }

    private static final class Collector implements DxfBlocks.Sink {
        final FloatBuffer points=new FloatBuffer();
        final IntBuffer triangles=new IntBuffer(),explicitEdges=new IntBuffer(),sourceLines=new IntBuffer(),coordinateSlots=new IntBuffer();
        final ArrayList<Face> faces=new ArrayList<>();
        final BitSet outlineOnlyTriangles=new BitSet();
        boolean polyface,polygonMesh,polylinePath,polylineClosed,meshClosedM,meshClosedN,solidProxy;
        int base,vertexCount,firstPathVertex=-1,previousPathVertex=-1,meshM,meshN,solids,resolvedSolids,approximateCurves;

        @Override public void accept(DxfBlocks.Placement p)throws IOException{
            if(!DxfBlocks.MODEL_LAYOUT.equalsIgnoreCase(p.layout))return;
            DxfBlocks.Record r=p.record;

            if("POLYLINE".equals(r.type)){
                finishGeometry();
                int flags=(int)r.number(70,0);
                polyface=(flags&64)!=0;
                polygonMesh=!polyface&&(flags&16)!=0;
                polylinePath=!polyface&&!polygonMesh;
                polylineClosed=(flags&1)!=0;
                meshClosedM=polygonMesh&&(flags&1)!=0;meshClosedN=polygonMesh&&(flags&32)!=0;
                meshM=polygonMesh?(int)r.number(71,0):0;meshN=polygonMesh?(int)r.number(72,0):0;
                if(polygonMesh&&(meshM<2||meshN<2||(long)meshM*meshN>MAX_VERTICES))throw new IOException("3B polygon mesh boyutları geçersiz veya sınırı aşıyor");
                base=points.n/3;vertexCount=0;firstPathVertex=-1;previousPathVertex=-1;faces.clear();
                return;
            }

            if("VERTEX".equals(r.type)&&(polyface||polygonMesh||polylinePath)){
                if(polyface){
                    int flags=(int)r.number(70,0);
                    if((flags&64)!=0){
                        addPoint(points,p,10,20,30);addSource(p,0);vertexCount++;
                    }else if((flags&128)!=0){
                        int[] indices=new int[4];for(int k=0;k<4;k++)indices[k]=Math.abs((int)r.number(71+k,0));
                        faces.add(new Face(indices,base));
                    }
                }else if(polygonMesh){
                    addPoint(points,p,10,20,30);addSource(p,0);vertexCount++;
                }else{
                    int index=points.n/3;
                    addPoint(points,p,10,20,30);addSource(p,0);
                    if(firstPathVertex<0)firstPathVertex=index;
                    if(previousPathVertex>=0)appendEdge(explicitEdges,previousPathVertex,index);
                    previousPathVertex=index;vertexCount++;
                }
                return;
            }

            // DxfBlocks intentionally drops SEQEND. Any next non-VERTEX entity closes the active path.
            finishGeometry();

            if("LINE".equals(r.type)){
                int start=points.n/3;
                addPoint(points,p,10,20,30);addSource(p,0);
                addPoint(points,p,11,21,31);addSource(p,1);
                appendEdge(explicitEdges,start,start+1);
            }else if("CIRCLE".equals(r.type)||"ARC".equals(r.type)){
                appendCurvedOutline(this,p,"CIRCLE".equals(r.type));
                approximateCurves++;
            }else if("3DFACE".equals(r.type)||"TRACE".equals(r.type)||"SOLID".equals(r.type)){
                boolean face3d="3DFACE".equals(r.type);
                double rawMask=face3d?r.number(70,0):0;
                if(!Double.isFinite(rawMask)||rawMask<0||rawMask>15||rawMask!=(int)rawMask)
                    throw new IOException("3DFACE görünmez kenar bayrağı geçersiz");
                int hidden=(int)rawMask;
                int start=points.n/3,firstTriangle=triangles.n/3;
                for(int k=0;k<3;k++){addPoint(points,p,10+k,20+k,30+k);addSource(p,k);}
                boolean quad=r.has(13)&&r.has(23);
                if(quad){
                    addPoint(points,p,13,23,33);addSource(p,3);
                    appendTriangle(triangles,start,start+1,start+2);
                    appendTriangle(triangles,start,start+2,start+3);
                }else{
                    appendTriangle(triangles,start,start+1,start+2);
                }
                outlineOnlyTriangles.set(firstTriangle,triangles.n/3);
                // In the DXF spec, group-70 bits 1,2,4,8 hide edges
                // 1-2,2-3,3-4,4-1 respectively. When vertex 4 is omitted,
                // it is equal to vertex 3, so the last edge is 3-1.
                if((hidden&1)==0)appendEdge(explicitEdges,start,start+1);
                if((hidden&2)==0)appendEdge(explicitEdges,start+1,start+2);
                if(quad){
                    if((hidden&4)==0)appendEdge(explicitEdges,start+2,start+3);
                    if((hidden&8)==0)appendEdge(explicitEdges,start+3,start);
                }else if((hidden&8)==0)appendEdge(explicitEdges,start+2,start);
            }else if("3DSOLID".equals(r.type)||"BODY".equals(r.type)||"REGION".equals(r.type)){
                solids++;
                ArrayList<String> chunks=new ArrayList<>();
                List<String> tags=r.tags();
                for(int i=r.from;i+1<r.to;i+=2){
                    int code;
                    try{code=Integer.parseInt(tags.get(i).trim());}catch(NumberFormatException ignored){continue;}
                    if(code==1||code==3)chunks.add(tags.get(i+1));
                }
                AcisSatMesh.Result acis=AcisSatMesh.parseDxfChunks(chunks);
                if(acis.hasGeometry()){
                    appendAcis(this,p,acis);
                    if(acis.complete)resolvedSolids++;
                }
            }
        }

        @Override public void finish()throws IOException{ finishGeometry(); }

        void finishGeometry()throws IOException{
            if(polyface){
                for(Face face:faces)appendFace(triangles,face.index,face.base,vertexCount);
            }else if(polygonMesh){
                appendPolygonMesh(triangles,base,vertexCount,meshM,meshN,meshClosedM,meshClosedN);
            }else if(polylinePath&&polylineClosed&&firstPathVertex>=0&&previousPathVertex>=0&&firstPathVertex!=previousPathVertex){
                appendEdge(explicitEdges,previousPathVertex,firstPathVertex);
            }
            polyface=false;polygonMesh=false;polylinePath=false;polylineClosed=false;meshClosedM=false;meshClosedN=false;faces.clear();
            firstPathVertex=-1;previousPathVertex=-1;vertexCount=0;meshM=meshN=0;
        }

        private void addSource(DxfBlocks.Placement p,int slot){
            boolean editable=p.directRoot&&p.spatial.isIdentity();
            sourceLines.add(editable?p.record.sourceStart:-1);
            coordinateSlots.add(slot);
        }
    }

    /**
     * Displays a DXF CIRCLE or ARC as a bounded 3D wire path. This is a
     * visual approximation; generated vertices have no editable DXF XYZ tags.
     * For an entity normal 210/220/230, center and angles are OCS values.
     * Compose the entity OCS basis with its containing INSERT's 3D transform.
     */
    private static void appendCurvedOutline(Collector c,DxfBlocks.Placement p,
            boolean circle)throws IOException{
        DxfBlocks.Record r=p.record;
        double radius=r.number(40,0);
        if(!(radius>0)||radius>1e9)
            throw new IOException("3B daire/yay yarıçapı geçersiz");
        double nx=r.number(210,0),ny=r.number(220,0),nz=r.number(230,1);
        DxfBlocks.SpatialTransform entityFrame;
        try{
            entityFrame=p.spatial.thenLocal(DxfBlocks.SpatialTransform.insert(
                0,0,0,1,1,1,0,0,0,0,nx,ny,nz));
        }catch(IllegalArgumentException badNormal){
            throw new IOException("3B daire/yay normal vektörü geçersiz",badNormal);
        }
        if(!entityFrame.isFinite())
            throw new IOException("3B daire/yay koordinat dönüşümü geçersiz");
        double x=r.number(10,0),y=r.number(20,0),z=r.number(30,0);
        double begin=circle?0:r.number(50,0);
        double end=circle?360:r.number(51,0);
        double sweep=360;
        if(!circle){
            sweep=(end-begin)%360d;
            if(sweep<0)sweep+=360d;
            if(sweep==0)
                throw new IOException("3B yay başlangıç/bitiş açıları çakışıyor");
        }
        // At most 48 straight segments per full circle (7.5 degrees each);
        // bounded allocation even for giant drawings. Curvature/arc length
        // measurements from tessellation are NOT exact CAD measurements.
        int segments=Math.max(1,(int)Math.ceil(sweep/7.5d));
        if(c.points.n/3+(long)segments+1>MAX_VERTICES||
           c.explicitEdges.n/2+(long)segments>MAX_EDGES)
            throw new IOException("3B eğri görüntüleme sınırı aşıldı");
        int first=c.points.n/3;
        int lastPoint=circle?segments-1:segments;
        for(int i=0;i<=lastPoint;i++){
            double theta=Math.toRadians(begin+sweep*((double)i/segments));
            double[] pt=entityFrame.point(x+radius*Math.cos(theta),
                                           y+radius*Math.sin(theta),z);
            for(int axis=0;axis<3;axis++){
                if(!Double.isFinite(pt[axis])||Math.abs(pt[axis])>1e9)
                    throw new IOException("3B eğri koordinat sınırı aşıldı");
                c.points.add((float)pt[axis]);
            }
            c.sourceLines.add(-1);
            c.coordinateSlots.add(-1);
            if(i>0)appendEdge(c.explicitEdges,first+i-1,first+i);
        }
        if(circle)appendEdge(c.explicitEdges,first+segments-1,first);
    }

    private static void appendAcis(Collector c,DxfBlocks.Placement p,AcisSatMesh.Result mesh)throws IOException{
        int base=c.points.n/3;
        for(int i=0;i+2<mesh.xyz.length;i+=3){
            if(c.points.n/3>=MAX_VERTICES)throw new IOException("ACIS 3B köşe sayısı sınırı aşıldı");
            double[] xyz=p.spatial.point(mesh.xyz[i],mesh.xyz[i+1],mesh.xyz[i+2]);
            double z=xyz[2];
            if(!Double.isFinite(xyz[0])||!Double.isFinite(xyz[1])||!Double.isFinite(z)||
                Math.abs(xyz[0])>1e9||Math.abs(xyz[1])>1e9||Math.abs(z)>1e9)
                throw new IOException("ACIS 3B koordinat sınırı aşıldı");
            c.points.add((float)xyz[0]);c.points.add((float)xyz[1]);c.points.add((float)z);
            c.sourceLines.add(-1);c.coordinateSlots.add(-1);
        }
        for(int i=0;i+2<mesh.triangles.length;i+=3)
            appendTriangle(c.triangles,base+mesh.triangles[i],base+mesh.triangles[i+1],base+mesh.triangles[i+2]);
        for(int i=0;i+1<mesh.edges.length;i+=2)
            appendEdge(c.explicitEdges,base+mesh.edges[i],base+mesh.edges[i+1]);
    }

    private static Extents readHeaderExtents(File file){
        double[] min={Double.NaN,Double.NaN,0d},max={Double.NaN,Double.NaN,0d};
        String target="";
        try(BufferedReader reader=new BufferedReader(new InputStreamReader(new FileInputStream(file),StandardCharsets.ISO_8859_1))){
            String codeLine,valueLine;
            while((codeLine=reader.readLine())!=null&&(valueLine=reader.readLine())!=null){
                int code;
                try{code=Integer.parseInt(codeLine.trim());}catch(NumberFormatException ignored){continue;}
                String value=valueLine.trim();
                if(code==9){
                    target=("$EXTMIN".equalsIgnoreCase(value)||"$EXTMAX".equalsIgnoreCase(value))?value.toUpperCase():"";
                    continue;
                }
                if(code==0&&"ENDSEC".equalsIgnoreCase(value)&&(!Double.isNaN(min[0])||!Double.isNaN(max[0])))break;
                if(target.isEmpty())continue;
                int axis=code==10?0:code==20?1:code==30?2:-1;
                if(axis<0)continue;
                try{
                    double parsed=Double.parseDouble(value);
                    if(!Double.isFinite(parsed)||Math.abs(parsed)>1e9)continue;
                    if("$EXTMIN".equals(target))min[axis]=parsed;else max[axis]=parsed;
                }catch(NumberFormatException ignored){}
            }
        }catch(IOException ignored){return null;}
        if(Double.isNaN(min[0])||Double.isNaN(min[1])||Double.isNaN(max[0])||Double.isNaN(max[1]))return null;
        if(Double.isNaN(min[2]))min[2]=0d;if(Double.isNaN(max[2]))max[2]=0d;
        if(max[0]<min[0]||max[1]<min[1]||max[2]<min[2])return null;
        if(max[0]-min[0]<1e-9&&max[1]-min[1]<1e-9&&max[2]-min[2]<1e-9)return null;
        return new Extents(min,max);
    }

    private static void addExtentProxy(Collector c,Extents e)throws IOException{
        int base=c.points.n/3;
        double x0=e.min[0],y0=e.min[1],z0=e.min[2],x1=e.max[0],y1=e.max[1],z1=e.max[2];
        if(Math.abs(z1-z0)<=1e-9){
            addRawPoint(c,x0,y0,z0);addRawPoint(c,x1,y0,z0);addRawPoint(c,x1,y1,z0);addRawPoint(c,x0,y1,z0);
            appendEdge(c.explicitEdges,base,base+1);appendEdge(c.explicitEdges,base+1,base+2);
            appendEdge(c.explicitEdges,base+2,base+3);appendEdge(c.explicitEdges,base+3,base);
            return;
        }
        addRawPoint(c,x0,y0,z0);addRawPoint(c,x1,y0,z0);addRawPoint(c,x1,y1,z0);addRawPoint(c,x0,y1,z0);
        addRawPoint(c,x0,y0,z1);addRawPoint(c,x1,y0,z1);addRawPoint(c,x1,y1,z1);addRawPoint(c,x0,y1,z1);
        int[][] edgePairs={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for(int[] pair:edgePairs)appendEdge(c.explicitEdges,base+pair[0],base+pair[1]);
    }

    private static void addRawPoint(Collector c,double x,double y,double z)throws IOException{
        if(c.points.n/3>=MAX_VERTICES)throw new IOException("3B köşe sayısı sınırı aşıldı");
        c.points.add((float)x);c.points.add((float)y);c.points.add((float)z);
        c.sourceLines.add(-1);c.coordinateSlots.add(-1);
    }

    private static final class Extents{
        final double[] min,max;
        Extents(double[] min,double[] max){this.min=min;this.max=max;}
    }

    private static void addPoint(FloatBuffer out,DxfBlocks.Placement p,int x,int y,int z)throws IOException{
        if(out.n/3>=MAX_VERTICES)throw new IOException("3B köşe sayısı sınırı aşıldı");
        DxfBlocks.Record r=p.record;
        double[] xyz=p.spatial.point(r.number(x,0),r.number(y,0),r.number(z,0));
        double c=xyz[2],a=xyz[0],b=xyz[1];
        if(!Double.isFinite(a)||!Double.isFinite(b)||!Double.isFinite(c)||Math.abs(a)>1e9||Math.abs(b)>1e9||Math.abs(c)>1e9)throw new IOException("3B koordinat sınırı aşıldı");
        out.add((float)a);out.add((float)b);out.add((float)c);
    }

    private static void edge(HashSet<Long> seen,IntBuffer out,int a,int b){
        if(a==b)return;long key=((long)Math.min(a,b)<<32)|(Math.max(a,b)&0xffffffffL);
        if(seen.add(key)){out.add(a);out.add(b);}
    }
    private static void appendFace(IntBuffer out,int[] face,int base,int count)throws IOException{
        int a=face[0],b=face[1],c=face[2],d=face[3];
        if(a<1||b<1||c<1||a>count||b>count||c>count)return;
        appendTriangle(out,base+a-1,base+b-1,base+c-1);
        if(d>=1&&d<=count&&d!=a&&d!=b&&d!=c)appendTriangle(out,base+a-1,base+c-1,base+d-1);
    }
    private static void appendPolygonMesh(IntBuffer out,int base,int vertexCount,int m,int n,boolean closedM,boolean closedN)throws IOException{
        if(m<2||n<2||vertexCount<m*n)return;
        boolean wrapM=closedM&&m>2,wrapN=closedN&&n>2;
        int mCells=wrapM?m:m-1,nCells=wrapN?n:n-1;
        for(int row=0;row<nCells;row++){
            int nextRow=(row+1)%n;
            for(int column=0;column<mCells;column++){
                int nextColumn=(column+1)%m;
                int a=base+row*m+column,b=base+row*m+nextColumn,c=base+nextRow*m+nextColumn,d=base+nextRow*m+column;
                appendTriangle(out,a,b,c);appendTriangle(out,a,c,d);
            }
        }
    }
    private static void appendTriangle(IntBuffer out,int a,int b,int c)throws IOException{
        if(out.n/3>=MAX_TRIANGLES)throw new IOException("3B yüzey sayısı sınırı aşıldı");
        out.add(a);out.add(b);out.add(c);
    }
    private static void appendEdge(IntBuffer out,int a,int b)throws IOException{
        if(out.n/2>=MAX_EDGES)throw new IOException("3B çizgi sayısı sınırı aşıldı");
        if(a==b)return;out.add(a);out.add(b);
    }

    private static final class IntBuffer{int[] a=new int[1024];int n;void add(int v){if(n==a.length)a=Arrays.copyOf(a,a.length*2);a[n++]=v;}int[] toArray(){return Arrays.copyOf(a,n);}}
    private static final class FloatBuffer{float[] a=new float[3072];int n;void add(float v){if(n==a.length)a=Arrays.copyOf(a,a.length*2);a[n++]=v;}float[] toArray(){return Arrays.copyOf(a,n);}}
    private static final class Face{final int[] index;final int base;Face(int[] index,int base){this.index=index;this.base=base;}}
}
