package com.musa.cad;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Reads DXF POLYLINE 3D vertices, polygon meshes, and polyface face indices.
 * Outputs a 2D XY projection while preserving the original XYZ coordinates.
 * This is not an ACIS/B-rep solid renderer.
 */
public final class DxfPolyline3D {
    public static final class Vertex {
        public final double x, y, z;
        public final int flags;
        public final int[] faceIndices;
        public final boolean hasPosition;
        private Vertex(double x,double y,double z,int flags,int[] faceIndices,boolean hasPosition){
            this.x=x;this.y=y;this.z=z;this.flags=flags;
            this.faceIndices=faceIndices;this.hasPosition=hasPosition;
        }
        private boolean isFace(){
            for(int index:faceIndices)if(index!=0)return true;
            // DXF polyface coordinate vertices carry 192=128+64.
            return (flags&128)!=0&&(flags&64)==0;
        }
    }
    public static final class Segment {
        public final Vertex start,end;
        private Segment(Vertex a,Vertex b){start=a;end=b;}
    }
    private DxfPolyline3D(){}

    public static Vertex readVertex(List<String> tags,int from,int to){
        if(tags==null||from<0||to>tags.size()||from>to||(to-from)%2!=0)return null;
        double[] coordinates=new double[3];
        boolean hasX=false,hasY=false;
        int flags=0;
        int[] indices=new int[4];
        for(int i=from;i+1<to;i+=2){
            int code;
            try{code=Integer.parseInt(tags.get(i).trim());}
            catch(RuntimeException exception){return null;}
            String value=tags.get(i+1).trim();
            try{
                if(code==70)flags=Integer.parseInt(value);
                else if(code>=71&&code<=74)indices[code-71]=Integer.parseInt(value);
                else if(code==10||code==20||code==30){
                    double number=Double.parseDouble(value);
                    // Android renders with float matrices; silent overflow
                    // would make bounds/path invalid rather than show geometry.
                    if(!Double.isFinite(number)||Math.abs(number)>Float.MAX_VALUE)return null;
                    coordinates[code/10-1]=number;
                    if(code==10)hasX=true;
                    if(code==20)hasY=true;
                }
            }catch(RuntimeException invalid){return null;}
        }
        Vertex result=new Vertex(coordinates[0],coordinates[1],coordinates[2],flags,indices,hasX&&hasY);
        return result.isFace()||result.hasPosition?result:null;
    }

    public static List<Segment> segments(int headerFlags,int m,int n,List<Vertex>vertices){
        if(vertices==null||vertices.isEmpty())return Collections.emptyList();
        if((headerFlags&64)!=0)return polyface(vertices);
        if((headerFlags&16)!=0)return polygonMesh(vertices,m,n,headerFlags);
        if((headerFlags&8)!=0)return threeDPolyline(vertices,(headerFlags&1)!=0);
        return Collections.emptyList();
    }

    private static List<Segment> threeDPolyline(List<Vertex>vertices,boolean closed){
        ArrayList<Segment>result=new ArrayList<>();
        Vertex first=null,previous=null;
        for(Vertex current:vertices){
            if(current==null||!current.hasPosition||current.isFace())
                return Collections.emptyList();
            if(first==null)first=current;
            if(previous!=null)addProjection(result,previous,current);
            previous=current;
        }
        if(closed&&first!=null&&previous!=first)addProjection(result,previous,first);
        return result;
    }

    private static List<Segment> polyface(List<Vertex>vertices){
        ArrayList<Vertex>points=new ArrayList<>();
        ArrayList<Vertex>faces=new ArrayList<>();
        for(Vertex v:vertices){
            if(v==null)return Collections.emptyList();
            if(v.isFace())faces.add(v);
            else if(v.hasPosition)points.add(v);
        }
        if(points.isEmpty()||faces.isEmpty())return Collections.emptyList();
        ArrayList<Segment>edges=new ArrayList<>();
        for(Vertex face:faces){
            int count=0;
            for(int idx:face.faceIndices)if(idx!=0)count++;
            if(count<3)continue;
            Vertex[] corners=new Vertex[count];
            boolean[] invisible=new boolean[count];
            boolean valid=true;
            int current=0;
            for(int idx:face.faceIndices){
                if(idx==0)continue;
                long absolute=Math.abs((long)idx);
                if(absolute<1||absolute>points.size()){valid=false;break;}
                corners[current]=points.get((int)absolute-1);
                invisible[current]=idx<0;  // Negative index hides this vertex's outgoing edge.
                current++;
            }
            if(!valid)continue;
            for(int i=0;i<count;i++)
                if(!invisible[i])addProjection(edges,corners[i],corners[(i+1)%count]);
        }
        return edges;
    }

    private static List<Segment> polygonMesh(List<Vertex>vertices,int m,int n,int flags){
        if(m<1||n<1||(long)m*n>500000||vertices.size()!=(long)m*n)
            return Collections.emptyList();
        for(Vertex v:vertices)
            if(v==null||!v.hasPosition||v.isFace())return Collections.emptyList();
        ArrayList<Segment>edges=new ArrayList<>();
        for(int row=0;row<m;row++)for(int col=0;col<n;col++){
            int idx=row*n+col;
            if(col+1<n)addProjection(edges,vertices.get(idx),vertices.get(idx+1));
            else if(n>2&&(flags&32)!=0)addProjection(edges,vertices.get(idx),vertices.get(row*n));
            if(row+1<m)addProjection(edges,vertices.get(idx),vertices.get(idx+n));
            else if(m>2&&(flags&1)!=0)addProjection(edges,vertices.get(idx),vertices.get(col));
        }
        return edges;
    }
    private static void addProjection(List<Segment>segments,Vertex start,Vertex end){
        if(start.x!=end.x||start.y!=end.y)
            segments.add(new Segment(start,end));
    }
}
