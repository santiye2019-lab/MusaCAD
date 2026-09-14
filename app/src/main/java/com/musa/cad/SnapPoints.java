package com.musa.cad;

/** Candidates use drawing-image coordinates; the hit radius is in screen pixels. */
public final class SnapPoints {
    public static int nearest(float[] points,float x,float y,float scale,float radius){
        if(points==null||points.length%2!=0||!Float.isFinite(x)||!Float.isFinite(y)||
            !Float.isFinite(scale)||scale<=0||!Float.isFinite(radius)||radius<=0)return -1;
        double limit=(double)radius/scale;
        double best=limit*limit;
        int found=-1;
        for(int i=0;i<points.length;i+=2){
            if(!Float.isFinite(points[i])||!Float.isFinite(points[i+1]))continue;
            double dx=(double)points[i]-x,dy=(double)points[i+1]-y,d=dx*dx+dy*dy;
            if(d<=best&&(found<0||d<best)){found=i;best=d;}
        }
        return found;
    }
    /** Immutable spatial index, prepared by the loader rather than the UI thread. */
    public static final class Index {
        private static final int GRID=64;
        private final float[] points;
        private final int[] offsets=new int[GRID*GRID+1],order;
        private final double minX,minY,width,height;
        public Index(float[] source)throws java.io.InterruptedIOException {
            points=source==null?new float[0]:source.clone();
            if(points.length%2!=0)throw new IllegalArgumentException("Coordinate pairs required");
            double x0=Double.POSITIVE_INFINITY,y0=x0,x1=Double.NEGATIVE_INFINITY,y1=x1;
            for(int i=0;i<points.length;i+=2){
                if((i&8191)==0)cancelled();
                if(!Float.isFinite(points[i])||!Float.isFinite(points[i+1]))continue;
                x0=Math.min(x0,points[i]);x1=Math.max(x1,points[i]);
                y0=Math.min(y0,points[i+1]);y1=Math.max(y1,points[i+1]);
            }
            minX=Double.isFinite(x0)?x0:0;minY=Double.isFinite(y0)?y0:0;
            width=Double.isFinite(x1)?Math.max(1,x1-minX):1;
            height=Double.isFinite(y1)?Math.max(1,y1-minY):1;
            for(int i=0;i<points.length;i+=2){
                if((i&8191)==0)cancelled();
                if(Float.isFinite(points[i])&&Float.isFinite(points[i+1]))offsets[cell(points[i],points[i+1])+1]++;
            }
            for(int i=1;i<offsets.length;i++)offsets[i]+=offsets[i-1];
            order=new int[offsets[offsets.length-1]];int[] next=offsets.clone();
            for(int i=0;i<points.length;i+=2){
                if((i&8191)==0)cancelled();
                if(Float.isFinite(points[i])&&Float.isFinite(points[i+1]))order[next[cell(points[i],points[i+1])]++]=i;
            }
        }
        private static void cancelled()throws java.io.InterruptedIOException {
            if(Thread.currentThread().isInterrupted())throw new java.io.InterruptedIOException("Yükleme iptal edildi");
        }
        private static int axis(double value,double min,double size){return Math.max(0,Math.min(GRID-1,(int)((value-min)/size*GRID)));}
        private int cell(double x,double y){return axis(y,minY,height)*GRID+axis(x,minX,width);}
        public float coordinate(int index){return points[index];}
        public int nearest(float x,float y,float scale,float radius){
            if(!Float.isFinite(x)||!Float.isFinite(y)||!Float.isFinite(scale)||scale<=0||!Float.isFinite(radius)||radius<=0)return -1;
            double limit=(double)radius/scale,best=limit*limit;int found=-1;
            int left=axis(x-limit,minX,width),right=axis(x+limit,minX,width);
            int top=axis(y-limit,minY,height),bottom=axis(y+limit,minY,height);
            for(int row=top;row<=bottom;row++)for(int col=left;col<=right;col++){
                int cell=row*GRID+col;
                for(int j=offsets[cell];j<offsets[cell+1];j++){
                    int i=order[j];double dx=(double)points[i]-x,dy=(double)points[i+1]-y,d=dx*dx+dy*dy;
                    if(d<=best&&(found<0||d<best||i<found&&d==best)){best=d;found=i;}
                }
            }
            return found;
        }
    }
    private SnapPoints(){}
}
