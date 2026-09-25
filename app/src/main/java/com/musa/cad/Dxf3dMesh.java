package com.musa.cad;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;

/**
 * Bounded XYZ geometry reader for MusaCAD 3D.
 * Expands ordinary INSERT blocks and supports:
 * 3DFACE, polyface, LINE, ordinary/3D POLYLINE, TRACE and SOLID geometry.
 */
public final class Dxf3dMesh {
    private static final int MAX_VERTICES=2_000_000,MAX_TRIANGLES=4_000_000,MAX_EDGES=4_000_000;
    public final float[] xyz;
    public final int[] triangles,edges,sourceLines,coordinateSlots;
    public final float cx,cy,cz,radius;
    public final int unsupportedSolids;

    private Dxf3dMesh(float[] xyz,int[] triangles,int[] explicitEdges,int[] sourceLines,int[] coordinateSlots,int unsupportedSolids){
        this.xyz=xyz;this.triangles=triangles;this.sourceLines=sourceLines;this.coordinateSlots=coordinateSlots;this.unsupportedSolids=unsupportedSolids;
        HashSet<Long> seen=new HashSet<>();IntBuffer lines=new IntBuffer();
        for(int i=0;i+1<explicitEdges.length;i+=2)edge(seen,lines,explicitEdges[i],explicitEdges[i+1]);
        for(int i=0;i+2<triangles.length;i+=3){
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
        cx=(minX+maxX)*.5f;cy=(minY+maxY)*.5f;cz=(minZ+maxZ)*.5f;
        radius=Math.max(1e-6f,(float)Math.sqrt((maxX-minX)*(maxX-minX)+(maxY-minY)*(maxY-minY)+(maxZ-minZ)*(maxZ-minZ))*.5f);
    }

    public static Dxf3dMesh read(File file)throws IOException{
        Collector collector=new Collector();
        DxfBlocks.expand(file,StandardCharsets.UTF_8,collector);
        collector.finishGeometry();
        if(collector.polyface)throw new IOException("3B polyface nesnesinin sonu bulunamadı");
        if(collector.points.n==0||(collector.triangles.n==0&&collector.explicitEdges.n==0))
            throw new IOException(collector.solids>0?"Katı 3B nesneler için geometri çözümleyici gerekli":"Bu çizimde desteklenen 3B veya çizgisel geometri bulunamadı");
        return new Dxf3dMesh(
            collector.points.toArray(),collector.triangles.toArray(),collector.explicitEdges.toArray(),
            collector.sourceLines.toArray(),collector.coordinateSlots.toArray(),collector.solids
        );
    }

    private static final class Collector implements DxfBlocks.Sink {
        final FloatBuffer points=new FloatBuffer();
        final IntBuffer triangles=new IntBuffer(),explicitEdges=new IntBuffer(),sourceLines=new IntBuffer(),coordinateSlots=new IntBuffer();
        final ArrayList<Face> faces=new ArrayList<>();
        boolean polyface,polylinePath,polylineClosed;
        int base,vertexCount,firstPathVertex=-1,previousPathVertex=-1,solids;

        @Override public void accept(DxfBlocks.Placement p)throws IOException{
            if(!DxfBlocks.MODEL_LAYOUT.equalsIgnoreCase(p.layout))return;
            DxfBlocks.Record r=p.record;

            if("POLYLINE".equals(r.type)){
                finishGeometry();
                int flags=(int)r.number(70,0);
                polyface=(flags&64)!=0;
                boolean polygonMesh=(flags&16)!=0;
                polylinePath=!polyface&&!polygonMesh;
                polylineClosed=(flags&1)!=0;
                base=points.n/3;vertexCount=0;firstPathVertex=-1;previousPathVertex=-1;faces.clear();
                return;
            }

            if("VERTEX".equals(r.type)&&(polyface||polylinePath)){
                if(polyface){
                    int flags=(int)r.number(70,0);
                    if((flags&64)!=0){
                        addPoint(points,p,10,20,30);addSource(p,0);vertexCount++;
                    }else if((flags&128)!=0){
                        int[] indices=new int[4];for(int k=0;k<4;k++)indices[k]=Math.abs((int)r.number(71+k,0));
                        faces.add(new Face(indices,base));
                    }
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
            }else if("3DFACE".equals(r.type)||"TRACE".equals(r.type)||"SOLID".equals(r.type)){
                int start=points.n/3;
                for(int k=0;k<3;k++){addPoint(points,p,10+k,20+k,30+k);addSource(p,k);}
                if(r.has(13)&&r.has(23)){
                    addPoint(points,p,13,23,33);addSource(p,3);
                    appendTriangle(triangles,start,start+1,start+2);appendTriangle(triangles,start,start+2,start+3);
                }else appendTriangle(triangles,start,start+1,start+2);
            }else if("3DSOLID".equals(r.type)||"BODY".equals(r.type)||"REGION".equals(r.type)){
                solids++;
            }
        }

        @Override public void finish()throws IOException{ finishGeometry(); }

        void finishGeometry()throws IOException{
            if(polyface){
                for(Face face:faces)appendFace(triangles,face.index,face.base,vertexCount);
            }else if(polylinePath&&polylineClosed&&firstPathVertex>=0&&previousPathVertex>=0&&firstPathVertex!=previousPathVertex){
                appendEdge(explicitEdges,previousPathVertex,firstPathVertex);
            }
            polyface=false;polylinePath=false;polylineClosed=false;faces.clear();
            firstPathVertex=-1;previousPathVertex=-1;vertexCount=0;
        }

        private void addSource(DxfBlocks.Placement p,int slot){
            boolean editable=p.directRoot&&p.transform.isIdentity();
            sourceLines.add(editable?p.record.sourceStart:-1);
            coordinateSlots.add(slot);
        }
    }

    private static void addPoint(FloatBuffer out,DxfBlocks.Placement p,int x,int y,int z)throws IOException{
        if(out.n/3>=MAX_VERTICES)throw new IOException("3B köşe sayısı sınırı aşıldı");
        DxfBlocks.Record r=p.record;
        double[] xy=p.transform.point(r.number(x,0),r.number(y,0));
        double c=r.number(z,0),a=xy[0],b=xy[1];
        if(Math.abs(a)>1e9||Math.abs(b)>1e9||Math.abs(c)>1e9)throw new IOException("3B koordinat sınırı aşıldı");
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
