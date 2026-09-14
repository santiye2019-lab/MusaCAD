package com.musa.cad;

/** Rectangular 2D paper-space VIEWPORT mapping for plan/top views. */
public final class DxfViewport {
    public static final class Spec {
        public final double centerX,centerY,width,height,viewCenterX,viewCenterY,viewHeight,twist;
        public final int id,status;
        public final double dirX,dirY,dirZ;
        Spec(double cx,double cy,double w,double h,double vx,double vy,double vh,double twist,
             int id,int status,double dx,double dy,double dz){
            centerX=cx;centerY=cy;width=w;height=h;viewCenterX=vx;viewCenterY=vy;viewHeight=vh;this.twist=twist;
            this.id=id;this.status=status;dirX=dx;dirY=dy;dirZ=dz;
        }
        /** Paper-space affine [a,b,c,d,x,y] mapping model XY into this viewport. */
        public double[] matrix(){
            if(!supported())throw new IllegalStateException("Unsupported viewport");
            double scale=height/viewHeight;
            double angle=-twist,cs=Math.cos(angle),sn=Math.sin(angle);
            double a=scale*cs,c=-scale*sn,b=scale*sn,d=scale*cs;
            double tx=centerX-a*viewCenterX-c*viewCenterY;
            double ty=centerY-b*viewCenterX-d*viewCenterY;
            return new double[]{a,b,c,d,tx,ty};
        }
        public double left(){return centerX-width*.5;}
        public double right(){return centerX+width*.5;}
        public double bottom(){return centerY-height*.5;}
        public double top(){return centerY+height*.5;}
        public boolean supported(){
            if(id<=1||status==0||!(width>0)||!(height>0)||!(viewHeight>0))return false;
            double len=Math.sqrt(dirX*dirX+dirY*dirY+dirZ*dirZ);
            return len>1e-12&&Math.abs(dirX/len)<1e-6&&Math.abs(dirY/len)<1e-6&&dirZ/len>0.999999;
        }
    }

    public static Spec of(double centerX,double centerY,double width,double height,
                          double viewCenterX,double viewCenterY,double viewHeight,double twistRadians,
                          int id,int status,double dirX,double dirY,double dirZ){
        return new Spec(centerX,centerY,Math.abs(width),Math.abs(height),viewCenterX,viewCenterY,
            Math.abs(viewHeight),twistRadians,id,status,dirX,dirY,dirZ);
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
