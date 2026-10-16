package com.musa.cad;

/**
 * Conservative, allocation-free prefilter for 2D source-entity picking.
 * Returns false only when the CAD primitive cannot be within tolerance.
 * Non-finite or unknown geometry falls back to the existing exact hit test.
 */
public final class CadPickBounds {
    private CadPickBounds(){}

    public static boolean mightHit(CadEdit edit,float x,float y,float tolerance){
        if(edit==null||!Float.isFinite(x)||!Float.isFinite(y)
            ||!Float.isFinite(tolerance)||tolerance<0f)return true;
        float[] xy=edit.xy;
        if(xy==null||xy.length<2)return true;
        float minX=Float.POSITIVE_INFINITY,minY=Float.POSITIVE_INFINITY;
        float maxX=Float.NEGATIVE_INFINITY,maxY=Float.NEGATIVE_INFINITY;

        switch(edit.type){
            case XLINE:
            case ELLIPSE:
                // Unbounded line or potentially skewed ellipse: avoid any
                // incorrect rejection from finite/axis-aligned approximation.
                return true;
            case CIRCLE:{
                if(xy.length<4)return true;
                float r=(float)Math.hypot(xy[2]-xy[0],xy[3]-xy[1]);
                if(!Float.isFinite(r))return true;
                minX=xy[0]-r;maxX=xy[0]+r;
                minY=xy[1]-r;maxY=xy[1]+r;
                break;
            }
            case ARC:{
                if(xy.length<8)return true;
                float r=(float)Math.hypot(xy[0]-xy[6],xy[1]-xy[7]);
                if(!Float.isFinite(r))return true;
                minX=xy[6]-r;maxX=xy[6]+r;
                minY=xy[7]-r;maxY=xy[7]+r;
                break;
            }
            default:
                for(int i=0;i+1<xy.length;i+=2){
                    if(!Float.isFinite(xy[i])||!Float.isFinite(xy[i+1]))return true;
                    minX=Math.min(minX,xy[i]);maxX=Math.max(maxX,xy[i]);
                    minY=Math.min(minY,xy[i+1]);maxY=Math.max(maxY,xy[i+1]);
                }
        }
        if(!Float.isFinite(minX)||!Float.isFinite(minY)
            ||!Float.isFinite(maxX)||!Float.isFinite(maxY))return true;
        // Compare using double arithmetic so a very large drawing coordinate
        // cannot overflow a float during the tolerance expansion.
        return (double)x+tolerance>=minX && (double)x-tolerance<=maxX
            && (double)y+tolerance>=minY && (double)y-tolerance<=maxY;
    }
}
