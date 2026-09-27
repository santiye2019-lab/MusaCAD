package com.musa.cad;

import java.util.List;

/** Applies the exact planar OCS -> WCS text transform for +Z/-Z extrusion normals. */
public final class DxfTextOcs {
    public static final class Placement {
        public final float x,y,x2,y2,angleDegrees;
        public final int generationFlags;
        public final boolean negativeZ,planar;
        Placement(float x,float y,float x2,float y2,float angle,int flags,boolean negativeZ,boolean planar){
            this.x=x;this.y=y;this.x2=x2;this.y2=y2;angleDegrees=angle;generationFlags=flags;this.negativeZ=negativeZ;this.planar=planar;
        }
    }

    public static Placement resolve(List<String> tags,int from,int to,float x,float y,float x2,float y2,float angleDegrees,int generationFlags){
        double nx=number(tags,from,to,210,0d),ny=number(tags,from,to,220,0d),nz=number(tags,from,to,230,1d);
        double length=Math.sqrt(nx*nx+ny*ny+nz*nz);
        if(!Double.isFinite(length)||length<1e-12)return new Placement(x,y,x2,y2,angleDegrees,generationFlags,false,false);
        nx/=length;ny/=length;nz/=length;
        boolean positive=Math.abs(nx)<1e-8&&Math.abs(ny)<1e-8&&nz>.999999;
        boolean negative=Math.abs(nx)<1e-8&&Math.abs(ny)<1e-8&&nz<-.999999;
        if(positive)return new Placement(x,y,x2,y2,angleDegrees,generationFlags,false,true);
        if(negative){
            // For OCS normal (0,0,-1), OCS X maps to -WCS X.
            // diag(-1,1) * R(angle) == R(-angle) * diag(-1,1), so toggle
            // the TEXT backward bit as well as reflecting alignment points.
            return new Placement(-x,y,-x2,y2,normalize(-angleDegrees),generationFlags^2,true,true);
        }
        // A tilted text plane needs a full projected 3D basis. Preserve the raw 2D
        // placement instead of inventing a lossy shear in the 2D renderer.
        return new Placement(x,y,x2,y2,angleDegrees,generationFlags,false,false);
    }

    private static double number(List<String> tags,int from,int to,int wanted,double fallback){
        if(tags==null)return fallback;
        try{
            for(int p=Math.max(0,from);p+1<to&&p+1<tags.size();p+=2)
                if(Integer.parseInt(tags.get(p).trim())==wanted){
                    double v=Double.parseDouble(tags.get(p+1).trim());
                    return Double.isFinite(v)?v:fallback;
                }
        }catch(Exception ignored){}
        return fallback;
    }
    private static float normalize(float degrees){
        if(!Float.isFinite(degrees))return 0f;
        float value=degrees%360f;if(value<=-180f)value+=360f;if(value>180f)value-=360f;return value;
    }
    private DxfTextOcs(){}
}
