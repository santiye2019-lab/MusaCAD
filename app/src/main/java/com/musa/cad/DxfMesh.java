package com.musa.cad;

import java.util.ArrayList;
import java.util.HashSet;

/** Pure wireframe extraction for modern DXF MESH entities. */
public final class DxfMesh {
    /**
     * verticesXY is packed x/y. faceItems uses DXF's [count,index...] stream.
     * edgeIndices is an optional packed [a,b,a,b...] explicit edge list.
     */
    public static double[] wireframe(double[] verticesXY,int[] faceItems,int[] edgeIndices){
        if(verticesXY==null||verticesXY.length<4||verticesXY.length%2!=0)return new double[0];
        int vertexCount=verticesXY.length/2;ArrayList<Double> out=new ArrayList<>();HashSet<Long> seen=new HashSet<>();
        if(faceItems!=null){
            int i=0;
            while(i<faceItems.length){
                int n=faceItems[i++];if(n<2||n>100000||i+n>faceItems.length)break;
                int first=faceItems[i],prev=first;
                for(int k=1;k<n;k++){int next=faceItems[i+k];edge(out,seen,verticesXY,vertexCount,prev,next);prev=next;}
                if(n>2)edge(out,seen,verticesXY,vertexCount,prev,first);i+=n;
            }
        }
        if(edgeIndices!=null)for(int i=0;i+1<edgeIndices.length;i+=2)edge(out,seen,verticesXY,vertexCount,edgeIndices[i],edgeIndices[i+1]);
        double[] result=new double[out.size()];for(int i=0;i<result.length;i++)result[i]=out.get(i);return result;
    }

    private static void edge(ArrayList<Double> out,HashSet<Long> seen,double[] xy,int count,int a,int b){
        if(a<0||b<0||a>=count||b>=count||a==b)return;int lo=Math.min(a,b),hi=Math.max(a,b);long key=((long)lo<<32)|(hi&0xffffffffL);if(!seen.add(key))return;
        double x1=xy[a*2],y1=xy[a*2+1],x2=xy[b*2],y2=xy[b*2+1];
        if(!finite(x1,y1,x2,y2)||Math.hypot(x2-x1,y2-y1)<1e-12)return;
        out.add(x1);out.add(y1);out.add(x2);out.add(y2);
    }
    private static boolean finite(double... values){for(double v:values)if(!Double.isFinite(v))return false;return true;}
    private DxfMesh(){}
}
