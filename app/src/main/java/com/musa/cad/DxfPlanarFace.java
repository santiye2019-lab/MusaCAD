package com.musa.cad;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * DXF SOLID/TRACE/3DFACE perimeter in the drawing's XY projection.
 *
 * DXF SOLID and TRACE use corner order 1,2,4,3 (not 1,2,3,4).
 * 3DFACE uses 1,2,3,4, and group 70 masks its four invisible edges.
 *
 * This helper only computes 2D outlines. It does NOT interpret ACIS solids,
 * construct 3D faces, or claim that projected faces are a complete 3D model.
 */
public final class DxfPlanarFace {
    public static final class Point {
        public final double x, y, z;
        Point(double x, double y, double z) { this.x=x; this.y=y; this.z=z; }
    }
    public static final class Edge {
        public final Point a, b;
        Edge(Point a,Point b) { this.a=a; this.b=b; }
    }
    public static final class Geometry {
        public final List<Point> outline;
        public final List<Edge> visibleEdges;
        Geometry(List<Point> outline,List<Edge> edges) {
            this.outline=Collections.unmodifiableList(outline);
            this.visibleEdges=Collections.unmodifiableList(edges);
        }
    }
    private DxfPlanarFace(){}

    public static Geometry parse(String entityType,List<String> tags,int from,int to){
        final boolean face="3DFACE".equals(entityType);
        if(!face&&!"SOLID".equals(entityType)&&!"TRACE".equals(entityType))return null;
        if(tags==null||from<0||to>tags.size()||from>to)return null;
        double[][] xyz=new double[4][3];
        boolean[][] has=new boolean[4][2];
        int hidden=0;
        for(int i=from;i+1<to;i+=2){
            int code;
            try{code=Integer.parseInt(tags.get(i).trim());}
            catch(RuntimeException ignored){continue;}
            String raw=tags.get(i+1);
            if(code==70&&face) {
                try{hidden=Integer.parseInt(raw.trim());}catch(RuntimeException ignored){}
                continue;
            }
            int coordinate=-1,axis=-1;
            if(code>=10&&code<=13){coordinate=code-10;axis=0;}
            else if(code>=20&&code<=23){coordinate=code-20;axis=1;}
            else if(code>=30&&code<=33){coordinate=code-30;axis=2;}
            if(coordinate<0)continue;
            try{
                double value=Double.parseDouble(raw.trim());
                if(!Double.isFinite(value))return null;
                xyz[coordinate][axis]=value;
                if(axis<2)has[coordinate][axis]=true;
            }catch(RuntimeException invalid){return null;}
        }
        for(int n=0;n<3;n++)if(!has[n][0]||!has[n][1])return null;
        Point[] vertices=new Point[4];
        for(int n=0;n<3;n++)vertices[n]=new Point(xyz[n][0],xyz[n][1],xyz[n][2]);
        boolean fourth=has[3][0]&&has[3][1];
        if(has[3][0]!=has[3][1])return null;
        vertices[3]=fourth?new Point(xyz[3][0],xyz[3][1],xyz[3][2]):vertices[2];
        int[] order=face?new int[]{0,1,2,3}:new int[]{0,1,3,2};
        ArrayList<Point> outline=new ArrayList<>(4);
        ArrayList<Edge> edges=new ArrayList<>(4);
        for(int side=0;side<4;side++){
            Point a=vertices[order[side]],b=vertices[order[(side+1)%4]];
            if(!sameXY(a,b)&&(face&&(hidden&(1<<side))==0||!face))
                edges.add(new Edge(a,b));
            if(outline.isEmpty()||!sameXY(outline.get(outline.size()-1),a))
                outline.add(a);
        }
        if(outline.size()>1&&sameXY(outline.get(0),outline.get(outline.size()-1)))
            outline.remove(outline.size()-1);
        if(outline.size()<3)return null;
        return new Geometry(outline,edges);
    }

    private static boolean sameXY(Point a,Point b){
        // Preserve tiny details at large coordinate magnitudes; do not
        // suppress a CAD feature using a scale-dependent tolerance.
        return a.x==b.x&&a.y==b.y;
    }
}
