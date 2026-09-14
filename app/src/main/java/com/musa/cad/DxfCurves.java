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
