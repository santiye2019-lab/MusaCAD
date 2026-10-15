package com.musa.cad;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

/**
 * Bounded diagnostic of INDEXED triangle mesh edge connectivity.
 *
 * Adjacent triangles must share the SAME vertex indices: geometrically equal
 * but duplicated vertices are not welded and will be reported as open. A
 * closed, consistently oriented edge network is ONLY a topology candidate:
 * it does not prove non-self-intersection, B-Rep validity, or watertight ACIS
 * solids, and it does not authorize volume/metraj claims.
 *
 * This runs on the CAD file-loading worker. Limit unique edge allocations to
 * protect phones from arbitrary multi-million-triangle DWGs.
 */
public final class Cad3dMeshTopology {
    public static final int MAX_AUDITED_TRIANGLES = 50000;
    private Cad3dMeshTopology(){}

    public static final class Result {
        public final boolean checked,closedOrientedByIndex;
        public final int triangles,boundaryEdges,orientationConflicts,
                nonManifoldEdges,degenerateTriangles,duplicateFaces;
        private Result(boolean checked,boolean closedOrientedByIndex,
                int triangles,int boundaryEdges,int orientationConflicts,
                int nonManifoldEdges,int degenerateTriangles,int duplicateFaces){
            this.checked=checked;
            this.closedOrientedByIndex=closedOrientedByIndex;
            this.triangles=triangles;
            this.boundaryEdges=boundaryEdges;
            this.orientationConflicts=orientationConflicts;
            this.nonManifoldEdges=nonManifoldEdges;
            this.degenerateTriangles=degenerateTriangles;
            this.duplicateFaces=duplicateFaces;
        }
    }

    private static final class FaceKey {
        final int a,b,c;
        FaceKey(int a,int b,int c){
            // Orientation-independent facet identity; a double-sided
            // zero-thickness triangle is not a watertight CAD solid.
            if(a>b){int tmp=a;a=b;b=tmp;}
            if(b>c){int tmp=b;b=c;c=tmp;}
            if(a>b){int tmp=a;a=b;b=tmp;}
            this.a=a;this.b=b;this.c=c;
        }
        @Override public int hashCode(){
            return (31*(31*a+b))+c;
        }
        @Override public boolean equals(Object other){
            if(!(other instanceof FaceKey))return false;
            FaceKey that=(FaceKey)other;
            return a==that.a&&b==that.b&&c==that.c;
        }
    }

    private static long edgeKey(int a,int b){
        int lo=Math.min(a,b),hi=Math.max(a,b);
        return ((long)lo<<32)|(hi&0xffffffffL);
    }

    /**
     * Stored edge state:
     * +1 / -1: one oriented edge (boundary)
     * 0: exactly two oppositely directed edges
     * +2 / -2: two edges pointing in the same direction (bad winding)
     * 3: three or more incidences (non-manifold), sticky state
     */
    private static void addEdge(Map<Long,Integer> edges,int a,int b){
        long key=edgeKey(a,b);
        int direction=a<b?1:-1;
        Integer previous=edges.get(key);
        if(previous==null)edges.put(key,direction);
        else if(previous==1||previous==-1)
            edges.put(key,previous+direction);
        else edges.put(key,3);
    }

    public static Result inspect(float[] xyz,int[] triangles){
        if(xyz==null||triangles==null||xyz.length%3!=0||
           triangles.length%3!=0)
            throw new IllegalArgumentException("invalid triangle mesh arrays");
        int count=triangles.length/3;
        if(count>MAX_AUDITED_TRIANGLES)
            return new Result(false,false,count,0,0,0,0,0);
        int vertexCount=xyz.length/3;
        // Avoid preallocating 3*triangleCount on untrusted inputs.
        Map<Long,Integer> edges=new HashMap<>();
        HashSet<FaceKey> faces=new HashSet<>();
        int degenerate=0,duplicates=0;
        for(int i=0;i<triangles.length;i+=3){
            int a=triangles[i],b=triangles[i+1],c=triangles[i+2];
            if(a<0||b<0||c<0||a>=vertexCount||
               b>=vertexCount||c>=vertexCount)
                throw new IllegalArgumentException("out-of-range mesh vertex");
            int ai=3*a,bi=3*b,ci=3*c;
            for(int axis=0;axis<3;axis++){
                if(!Float.isFinite(xyz[ai+axis])||
                   !Float.isFinite(xyz[bi+axis])||
                   !Float.isFinite(xyz[ci+axis]))
                    throw new IllegalArgumentException("nonfinite 3D vertex");
            }
            if(a==b||b==c||c==a){
                degenerate++;continue;
            }
            double ux=(double)xyz[bi]-xyz[ai],
                uy=(double)xyz[bi+1]-xyz[ai+1],
                uz=(double)xyz[bi+2]-xyz[ai+2];
            double vx=(double)xyz[ci]-xyz[ai],
                vy=(double)xyz[ci+1]-xyz[ai+1],
                vz=(double)xyz[ci+2]-xyz[ai+2];
            double cx=uy*vz-uz*vy,cy=uz*vx-ux*vz,cz=ux*vy-uy*vx;
            if(cx==0&&cy==0&&cz==0){
                degenerate++;continue;
            }
            if(!faces.add(new FaceKey(a,b,c)))duplicates++;
            addEdge(edges,a,b);
            addEdge(edges,b,c);
            addEdge(edges,c,a);
        }
        int boundary=0,winding=0,nonManifold=0;
        for(int state:edges.values()){
            if(state==1||state==-1)boundary++;
            else if(state==2||state==-2)winding++;
            else if(state==3)nonManifold++;
        }
        boolean closed=count>0&&degenerate==0&&boundary==0&&
                winding==0&&nonManifold==0&&duplicates==0;
        return new Result(true,closed,count,boundary,winding,nonManifold,degenerate,duplicates);
    }
}
