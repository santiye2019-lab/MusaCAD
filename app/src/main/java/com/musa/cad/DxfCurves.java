package com.musa.cad;

import java.util.ArrayList;

/** Geometry samplers used to render DXF curved entities without flattening them to crude chords. */
public final class DxfCurves {
    /**
     * Samples a DXF bulge polyline. bulge[i] belongs to the segment starting at vertex i.
     * A bulge is tan(includedAngle/4); zero means a straight segment.
     */
    public static double[] sampleBulgePolyline(double[] xs,double[] ys,double[] bulges,boolean closed){
        if(xs==null||ys==null||xs.length!=ys.length||xs.length<2)return new double[0];
        int n=xs.length;ArrayList<Double> out=new ArrayList<>();
        add(out,xs[0],ys[0]);
        int segments=closed?n:n-1;
        for(int i=0;i<segments;i++){
            int j=(i+1)%n;
            double b=bulges!=null&&i<bulges.length?bulges[i]:0d;
            addBulgeSegment(out,xs[i],ys[i],xs[j],ys[j],b);
        }
        return packed(out);
    }

    private static void addBulgeSegment(ArrayList<Double> out,double x1,double y1,double x2,double y2,double bulge){
        double dx=x2-x1,dy=y2-y1,chord=Math.hypot(dx,dy);
        if(!finite(x1,y1,x2,y2,bulge)||chord<1e-12||Math.abs(bulge)<1e-10){add(out,x2,y2);return;}
        double theta=4d*Math.atan(bulge);
        // Midpoint-to-center distance, signed by bulge. This form stays stable for semicircles.
        double h=chord*(1d-bulge*bulge)/(4d*bulge);
        double mx=(x1+x2)*.5d,my=(y1+y2)*.5d;
        double cx=mx-dy/chord*h,cy=my+dx/chord*h;
        double radius=Math.hypot(x1-cx,y1-cy),start=Math.atan2(y1-cy,x1-cx);
        if(!Double.isFinite(radius)||radius<1e-12){add(out,x2,y2);return;}
        int steps=Math.max(2,Math.min(96,(int)Math.ceil(Math.abs(theta)/(Math.PI/18d))));
        for(int s=1;s<=steps;s++){
            if(s==steps){add(out,x2,y2);continue;}
            double a=start+theta*s/steps;
            add(out,cx+radius*Math.cos(a),cy+radius*Math.sin(a));
        }
    }

    /** Samples a rational B-spline/NURBS using the DXF knot vector, degree and optional weights. */
    public static double[] sampleNurbs(int degree,double[] knots,double[] weights,double[] xs,double[] ys){
        if(xs==null||ys==null||xs.length!=ys.length||xs.length<2)return new double[0];
        int n=xs.length-1,p=degree;
        if(p<1||p>n||knots==null||knots.length<n+p+2)return new double[0];
        for(int i=1;i<knots.length;i++)if(!Double.isFinite(knots[i])||knots[i]<knots[i-1])return new double[0];
        double start=knots[p],end=knots[n+1];
        if(!Double.isFinite(start)||!Double.isFinite(end)||end<=start)return new double[0];
        int samples=Math.max(32,Math.min(1024,(n+1)*12));
        ArrayList<Double> out=new ArrayList<>((samples+1)*2);
        for(int i=0;i<=samples;i++){
            double u=i==samples?end:start+(end-start)*i/samples;
            double[] q=deBoor(u,p,knots,weights,xs,ys,n);
            if(q==null)return new double[0];
            add(out,q[0],q[1]);
        }
        return packed(out);
    }

    /**
     * Smooth fallback for SPLINE entities that provide fit points but no usable control
     * point/knot representation. It interpolates every fit point with a cubic Hermite
     * curve instead of joining them with visible straight chords. Optional DXF start/end
     * tangent directions are honored when present. NaN tangents mean automatic tangents.
     */
    public static double[] sampleFitSpline(double[] xs,double[] ys,boolean closed,
                                           double startTx,double startTy,double endTx,double endTy){
        if(xs==null||ys==null||xs.length!=ys.length||xs.length<2)return new double[0];
        int n=xs.length;for(int i=0;i<n;i++)if(!finite(xs[i],ys[i]))return new double[0];
        if(n==2&&!closed)return new double[]{xs[0],ys[0],xs[1],ys[1]};
        double[] tx=new double[n],ty=new double[n];
        for(int i=0;i<n;i++){
            if(closed){int prev=(i+n-1)%n,next=(i+1)%n;tx[i]=(xs[next]-xs[prev])*.5;ty[i]=(ys[next]-ys[prev])*.5;}
            else if(i==0){tx[i]=xs[1]-xs[0];ty[i]=ys[1]-ys[0];}
            else if(i==n-1){tx[i]=xs[n-1]-xs[n-2];ty[i]=ys[n-1]-ys[n-2];}
            else{tx[i]=(xs[i+1]-xs[i-1])*.5;ty[i]=(ys[i+1]-ys[i-1])*.5;}
        }
        if(!closed){
            double firstChord=Math.hypot(xs[1]-xs[0],ys[1]-ys[0]);double lastChord=Math.hypot(xs[n-1]-xs[n-2],ys[n-1]-ys[n-2]);
            double[] first=scaledDirection(startTx,startTy,firstChord);if(first!=null){tx[0]=first[0];ty[0]=first[1];}
            double[] last=scaledDirection(endTx,endTy,lastChord);if(last!=null){tx[n-1]=last[0];ty[n-1]=last[1];}
        }
        int segments=closed?n:n-1,steps=Math.max(8,Math.min(48,12));ArrayList<Double> out=new ArrayList<>((segments*steps+1)*2);add(out,xs[0],ys[0]);
        for(int i=0;i<segments;i++){
            int j=(i+1)%n;
            for(int k=1;k<=steps;k++){
                if(k==steps){add(out,xs[j],ys[j]);continue;}
                double u=k/(double)steps,u2=u*u,u3=u2*u;
                double h00=2*u3-3*u2+1,h10=u3-2*u2+u,h01=-2*u3+3*u2,h11=u3-u2;
                add(out,h00*xs[i]+h10*tx[i]+h01*xs[j]+h11*tx[j],h00*ys[i]+h10*ty[i]+h01*ys[j]+h11*ty[j]);
            }
        }
        return packed(out);
    }

    public static double[] sampleFitSpline(double[] xs,double[] ys,boolean closed){
        return sampleFitSpline(xs,ys,closed,Double.NaN,Double.NaN,Double.NaN,Double.NaN);
    }

    private static double[] scaledDirection(double x,double y,double length){
        if(!finite(x,y,length)||length<=1e-12)return null;double norm=Math.hypot(x,y);if(norm<=1e-12)return null;return new double[]{x/norm*length,y/norm*length};
    }

    private static double[] deBoor(double u,int p,double[] knots,double[] weights,double[] xs,double[] ys,int n){
        int k=findSpan(u,p,knots,n);
        double[][] d=new double[p+1][3];
        for(int j=0;j<=p;j++){
            int index=k-p+j;
            double w=weights!=null&&index<weights.length?weights[index]:1d;
            if(!Double.isFinite(w)||Math.abs(w)<1e-15)w=1d;
            if(!finite(xs[index],ys[index]))return null;
            d[j][0]=xs[index]*w;d[j][1]=ys[index]*w;d[j][2]=w;
        }
        for(int r=1;r<=p;r++){
            for(int j=p;j>=r;j--){
                int index=k-p+j;
                double den=knots[index+p-r+1]-knots[index];
                double alpha=Math.abs(den)<1e-15?0d:(u-knots[index])/den;
                if(alpha<0)alpha=0;else if(alpha>1)alpha=1;
                for(int c=0;c<3;c++)d[j][c]=(1d-alpha)*d[j-1][c]+alpha*d[j][c];
            }
        }
        double w=d[p][2];if(!Double.isFinite(w)||Math.abs(w)<1e-15)return null;
        double x=d[p][0]/w,y=d[p][1]/w;
        return finite(x,y)?new double[]{x,y}:null;
    }

    private static int findSpan(double u,int p,double[] knots,int n){
        if(u>=knots[n+1])return n;
        int low=p,high=n+1,mid=(low+high)/2;
        while(u<knots[mid]||u>=knots[mid+1]){
            if(u<knots[mid])high=mid;else low=mid;
            mid=(low+high)/2;
        }
        return mid;
    }

    private static boolean finite(double... values){for(double v:values)if(!Double.isFinite(v))return false;return true;}
    private static void add(ArrayList<Double> out,double x,double y){out.add(x);out.add(y);}
    private static double[] packed(ArrayList<Double> values){double[] r=new double[values.size()];for(int i=0;i<r.length;i++)r[i]=values.get(i);return r;}
    private DxfCurves(){}
}
