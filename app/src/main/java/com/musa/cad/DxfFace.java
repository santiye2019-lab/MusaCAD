package com.musa.cad;

import java.util.ArrayList;

/** Builds the visible 2D edge segments of a DXF 3DFACE entity. */
public final class DxfFace {
    /** Group 70 bits: 1/2/4/8 hide edges 1/2/3/4 respectively. */
    public static double[] visibleEdges(double[] xy,int flags){
        if(xy==null||xy.length<6)return new double[0];
        int vertices=Math.min(4,xy.length/2);if(vertices<3)return new double[0];
        double[] x=new double[4],y=new double[4];
        for(int i=0;i<vertices;i++){x[i]=xy[i*2];y[i]=xy[i*2+1];}
        if(vertices==3){x[3]=x[2];y[3]=y[2];vertices=4;}
        ArrayList<Double> out=new ArrayList<>(16);
        for(int edge=0;edge<4;edge++){
            if((flags&(1<<edge))!=0)continue;
            int next=(edge+1)&3;
            if(!finite(x[edge],y[edge],x[next],y[next]))continue;
            double dx=x[next]-x[edge],dy=y[next]-y[edge];
            if(dx*dx+dy*dy<1e-24)continue;
            out.add(x[edge]);out.add(y[edge]);out.add(x[next]);out.add(y[next]);
        }
        double[] result=new double[out.size()];for(int i=0;i<result.length;i++)result[i]=out.get(i);return result;
    }
    private static boolean finite(double... values){for(double v:values)if(!Double.isFinite(v))return false;return true;}
    private DxfFace(){}
}
