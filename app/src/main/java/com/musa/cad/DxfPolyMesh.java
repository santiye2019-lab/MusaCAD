package com.musa.cad;

import java.util.*;

/** Builds stable 2D wireframe segments for classic DXF polygon meshes and polyface meshes. */
public final class DxfPolyMesh {
    public static double[] polygonMesh(double[] xy,int mCount,int nCount,boolean closeM,boolean closeN){
        if(xy==null||(xy.length&1)!=0||mCount<=0||nCount<=0)return new double[0];
        long required=(long)mCount*nCount;if(required<=0||required>xy.length/2L)return new double[0];
        LinkedHashSet<String> seen=new LinkedHashSet<>();ArrayList<Double> out=new ArrayList<>();
        for(int m=0;m<mCount;m++)for(int n=0;n<nCount;n++){
            int here=m*nCount+n;
            if(n+1<nCount)add(out,seen,xy,here,here+1);else if(closeN&&nCount>2)add(out,seen,xy,here,m*nCount);
            if(m+1<mCount)add(out,seen,xy,here,(m+1)*nCount+n);else if(closeM&&mCount>2)add(out,seen,xy,here,n);
        }
        return packed(out);
    }

    /**
     * Face indices are 1-based as stored by DXF VERTEX groups 71..74.
     * A negative index suppresses the edge starting at that indexed vertex.
     */
    public static double[] polyface(double[] xy,List<int[]> faces){
        if(xy==null||(xy.length&1)!=0||faces==null||faces.isEmpty())return new double[0];
        int vertexCount=xy.length/2;LinkedHashSet<String> seen=new LinkedHashSet<>();ArrayList<Double> out=new ArrayList<>();
        for(int[] rawFace:faces){
            if(rawFace==null)continue;ArrayList<Integer> face=new ArrayList<>();
            for(int raw:rawFace)if(raw!=0)face.add(raw);
            if(face.size()<2)continue;
            for(int i=0;i<face.size();i++){
                int raw=face.get(i),nextRaw=face.get((i+1)%face.size());if(raw<0)continue;
                int a=Math.abs(raw)-1,b=Math.abs(nextRaw)-1;if(a<0||b<0||a>=vertexCount||b>=vertexCount||a==b)continue;
                add(out,seen,xy,a,b);
            }
        }
        return packed(out);
    }

    private static void add(ArrayList<Double> out,Set<String> seen,double[] xy,int a,int b){
        int lo=Math.min(a,b),hi=Math.max(a,b);String key=lo+":"+hi;if(!seen.add(key))return;
        double x1=xy[a*2],y1=xy[a*2+1],x2=xy[b*2],y2=xy[b*2+1];
        if(!Double.isFinite(x1)||!Double.isFinite(y1)||!Double.isFinite(x2)||!Double.isFinite(y2))return;
        out.add(x1);out.add(y1);out.add(x2);out.add(y2);
    }
    private static double[] packed(ArrayList<Double> values){double[] r=new double[values.size()];for(int i=0;i<r.length;i++)r[i]=values.get(i);return r;}
    private DxfPolyMesh(){}
}
