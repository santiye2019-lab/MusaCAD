package com.musa.cad;

/** Rectangular paper-space VIEWPORT mapping for orthographic views of the XY model plane. */
public final class DxfViewport {
    public static final class Spec {
        public final double centerX,centerY,width,height,viewCenterX,viewCenterY,viewHeight,twist;
        public final int id,status;
        public final double dirX,dirY,dirZ,targetX,targetY,targetZ;
        Spec(double cx,double cy,double w,double h,double vx,double vy,double vh,double twist,
             int id,int status,double dx,double dy,double dz,double tx,double ty,double tz){
            centerX=cx;centerY=cy;width=w;height=h;viewCenterX=vx;viewCenterY=vy;viewHeight=vh;this.twist=twist;
            this.id=id;this.status=status;dirX=dx;dirY=dy;dirZ=dz;targetX=tx;targetY=ty;targetZ=tz;
        }

        /** Paper-space affine [a,b,c,d,x,y] mapping model XY (Z=0) into this viewport. */
        public double[] matrix(){
            if(!supported())throw new IllegalStateException("Unsupported viewport");
            double len=Math.sqrt(dirX*dirX+dirY*dirY+dirZ*dirZ),nx=dirX/len,ny=dirY/len,nz=dirZ/len;
            double rx,ry,rz=0d;
            if(Math.abs(nx)<1d/64d&&Math.abs(ny)<1d/64d){rx=nz>=0?1d:-1d;ry=0d;}
            else{double rlen=Math.hypot(nx,ny);rx=-ny/rlen;ry=nx/rlen;}
            double ux=ny*rz-nz*ry,uy=nz*rx-nx*rz,uz=nx*ry-ny*rx;
            double ulen=Math.sqrt(ux*ux+uy*uy+uz*uz);ux/=ulen;uy/=ulen;uz/=ulen;

            double originU=-(targetX*rx+targetY*ry+targetZ*rz)-viewCenterX;
            double originV=-(targetX*ux+targetY*uy+targetZ*uz)-viewCenterY;
            double scale=height/viewHeight,angle=-twist,cs=Math.cos(angle),sn=Math.sin(angle);
            double a=scale*(cs*rx-sn*ux),c=scale*(cs*ry-sn*uy);
            double b=scale*(sn*rx+cs*ux),d=scale*(sn*ry+cs*uy);
            double tx=centerX+scale*(cs*originU-sn*originV);
            double ty=centerY+scale*(sn*originU+cs*originV);
            return new double[]{a,b,c,d,tx,ty};
        }
        public double left(){return centerX-width*.5;}
        public double right(){return centerX+width*.5;}
        public double bottom(){return centerY-height*.5;}
        public double top(){return centerY+height*.5;}
        public boolean supported(){
            if(id<=1||status==0||!(width>0)||!(height>0)||!(viewHeight>0))return false;
            double len=Math.sqrt(dirX*dirX+dirY*dirY+dirZ*dirZ);if(!(len>1e-12))return false;
            // MusaCAD still renders model entities as a 2D XY plane. A pure side view makes
            // that plane singular; oblique/isometric orthographic views with nonzero Z are safe.
            return Math.abs(dirZ/len)>1e-6;
        }
    }

    /** Backward-compatible top/oblique constructor with a zero WCS view target. */
    public static Spec of(double centerX,double centerY,double width,double height,
                          double viewCenterX,double viewCenterY,double viewHeight,double twistRadians,
                          int id,int status,double dirX,double dirY,double dirZ){
        return of(centerX,centerY,width,height,viewCenterX,viewCenterY,viewHeight,twistRadians,
            id,status,dirX,dirY,dirZ,0,0,0);
    }

    public static Spec of(double centerX,double centerY,double width,double height,
                          double viewCenterX,double viewCenterY,double viewHeight,double twistRadians,
                          int id,int status,double dirX,double dirY,double dirZ,
                          double targetX,double targetY,double targetZ){
        return new Spec(centerX,centerY,Math.abs(width),Math.abs(height),viewCenterX,viewCenterY,
            Math.abs(viewHeight),twistRadians,id,status,dirX,dirY,dirZ,targetX,targetY,targetZ);
    }

    public static double[] point(Spec spec,double x,double y){
        double[] m=spec.matrix();return new double[]{m[0]*x+m[2]*y+m[4],m[1]*x+m[3]*y+m[5]};
    }

    public static boolean contains(Spec spec,double paperX,double paperY,double tolerance){
        double t=Math.max(0,tolerance);
        return paperX>=spec.left()-t&&paperX<=spec.right()+t&&paperY>=spec.bottom()-t&&paperY<=spec.top()+t;
    }

    private DxfViewport(){}
}
