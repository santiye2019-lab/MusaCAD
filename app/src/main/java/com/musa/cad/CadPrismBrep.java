package com.musa.cad;

import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Narrow, deterministic first exact-topology modeling primitive: a watertight
 * linear extrusion of a validated simple planar polygon.
 *
 * This is a polygonal B-Rep shell (faces/edge incidence), NOT an ACIS kernel,
 * 3DSOLID encoder, spline/curved-surface Boolean engine or arbitrary DWG writer.
 * No Android UI integration is enabled until device/fidelity acceptance.
 */
public final class CadPrismBrep {
    private CadPrismBrep() {}
    private static final int MAX_RING = 4096;

    public static final class Solid {
        /** XYZ interleaved: first the low ring, then the high ring. */
        public final double[] xyz;
        /** Outward-oriented triangle vertex indices. */
        public final int[] triangles;
        public final double volume;
        public final int ringVertices;

        private Solid(double[] xyz, int[] triangles, double volume, int ringVertices) {
            this.xyz=xyz;this.triangles=triangles;this.volume=volume;
            this.ringVertices=ringVertices;
        }

        /** Every unoriented mesh edge must occur exactly twice, oppositely oriented. */
        public boolean isWatertight() {
            HashMap<Long, int[]> edges = new HashMap<>();
            for (int i=0;i<triangles.length;i+=3) {
                for (int j=0;j<3;j++) {
                    int a=triangles[i+j],b=triangles[i+(j+1)%3];
                    if (a==b || a<0 || b<0 || a>=xyz.length/3 || b>=xyz.length/3) return false;
                    int low=Math.min(a,b), high=Math.max(a,b);
                    long key=((long)low<<32)|(high&0xffffffffL);
                    int[] value=edges.get(key);
                    if (value==null) edges.put(key,new int[]{1,a<b?1:-1});
                    else {value[0]++;value[1]+=a<b?1:-1;}
                }
            }
            if (edges.isEmpty()) return false;
            for (int[] value:edges.values()) if(value[0]!=2||value[1]!=0)return false;
            return true;
        }

        /**
         * DXF 3DFACE mesh copy, not a native ACIS 3DSOLID.
         * Always emit a new file; caller must handle protected publication.
         */
        public void writeDxf(Writer out, String layer) throws IOException {
            if(out==null||layer==null||!layer.matches("[A-Za-z0-9_.-]{1,32}"))
                throw new IllegalArgumentException("Invalid DXF layer/target");
            validateBeforeWrite();
            tag(out,0,"SECTION");tag(out,2,"HEADER");
            tag(out,9,"$ACADVER");tag(out,1,"AC1015");
            tag(out,0,"ENDSEC");tag(out,0,"SECTION");tag(out,2,"ENTITIES");
            for(int i=0;i<triangles.length;i+=3){
                tag(out,0,"3DFACE");tag(out,8,layer);
                for(int j=0;j<4;j++){
                    int v=triangles[i+(j==3?2:j)]*3;
                    tag(out,10+j,Double.toString(xyz[v]));
                    tag(out,20+j,Double.toString(xyz[v+1]));
                    tag(out,30+j,Double.toString(xyz[v+2]));
                }
            }
            tag(out,0,"ENDSEC");tag(out,0,"EOF");
        }


        /**
         * Public XYZ/triangle arrays can be modified by a caller after extrusion.
         * Refuse unsafe geometry *before emitting the first byte* to a DXF writer.
         * This checks mesh topology, finite CAD coordinates, nondegenerate faces
         * and the signed volume previously established by the extrusion engine.
         */
        private void validateBeforeWrite() {
            int n=ringVertices;
            if(n<3 || n>MAX_RING || xyz==null || triangles==null ||
                    xyz.length!=6*n ||
                    triangles.length!=((n-2)*2+n*2)*3 ||
                    !Double.isFinite(volume) || volume<=0)
                throw new IllegalArgumentException("3D output mesh layout is invalid");
            for(double v:xyz)
                if(!bounded(v))throw new IllegalArgumentException("Non-finite/out-of-range 3D coordinate");
            if(!isWatertight())
                throw new IllegalArgumentException("3D output shell is not watertight");
            double ox=xyz[0],oy=xyz[1],oz=xyz[2],signedSix=0d;
            for(int i=0;i<triangles.length;i+=3) {
                int ia=triangles[i]*3,ib=triangles[i+1]*3,ic=triangles[i+2]*3;
                double ax=xyz[ia]-ox,ay=xyz[ia+1]-oy,az=xyz[ia+2]-oz;
                double bx=xyz[ib]-ox,by=xyz[ib+1]-oy,bz=xyz[ib+2]-oz;
                double cx=xyz[ic]-ox,cy=xyz[ic+1]-oy,cz=xyz[ic+2]-oz;
                double crossX=by*cz-bz*cy,crossY=bz*cx-bx*cz,crossZ=bx*cy-by*cx;
                if((crossX==0d&&crossY==0d&&crossZ==0d)||
                        !Double.isFinite(crossX)||!Double.isFinite(crossY)||
                        !Double.isFinite(crossZ))
                    throw new IllegalArgumentException("Degenerate 3D output triangle");
                signedSix+=ax*crossX+ay*crossY+az*crossZ;
            }
            double signedVolume=signedSix/6d;
            if(!Double.isFinite(signedVolume)||signedVolume<=0 ||
                    Math.abs(signedVolume-volume)>Math.max(1e-8,volume*1e-8))
                throw new IllegalArgumentException("3D output solid volume/orientation changed");
        }

        private static void tag(Writer out,int group,String value)throws IOException{
            out.write(Integer.toString(group));out.write("\n");out.write(value);out.write("\n");
        }
    }

    /** ringXY is a simple, hole-free polygon; height can be positive or negative. */
    public static Solid extrude(double[] ringXY,double baseZ,double height) {
        if(ringXY==null||ringXY.length<6||ringXY.length%2!=0||
                ringXY.length/2>MAX_RING||!bounded(baseZ)||!bounded(height)||
                Math.abs(height)<=1e-9||!bounded(baseZ+height))
            throw new IllegalArgumentException("Invalid extrusion arguments");
        int n=ringXY.length/2;
        double[] ring=Arrays.copyOf(ringXY,ringXY.length);
        double minX=Double.POSITIVE_INFINITY,minY=Double.POSITIVE_INFINITY;
        double maxX=Double.NEGATIVE_INFINITY,maxY=Double.NEGATIVE_INFINITY;
        for(int i=0;i<ring.length;i+=2){
            double x=ring[i],y=ring[i+1];
            if(!bounded(x)||!bounded(y))throw new IllegalArgumentException("Out-of-range vertex");
            minX=Math.min(minX,x);minY=Math.min(minY,y);
            maxX=Math.max(maxX,x);maxY=Math.max(maxY,y);
        }
        double scale=Math.max(maxX-minX,maxY-minY);
        if(scale<1e-9)throw new IllegalArgumentException("Collapsed polygon");
        double eps=Math.max(1e-18,1e-12*scale*scale);
        double area2=0;
        for(int i=0;i<n;i++){
            int j=(i+1)%n;
            double dx=ring[2*j]-ring[2*i],dy=ring[2*j+1]-ring[2*i+1];
            if(dx*dx+dy*dy<=eps)throw new IllegalArgumentException("Repeated neighbor vertices");
            area2+=(ring[2*i]-minX)*(ring[2*j+1]-minY)
                    -(ring[2*j]-minX)*(ring[2*i+1]-minY);
        }
        if(Math.abs(area2)<=eps)throw new IllegalArgumentException("Zero-area polygon");
        if(area2<0) {
            for(int i=0;i<n/2;i++){
                int j=n-1-i;
                for(int k=0;k<2;k++){
                    double temp=ring[2*i+k];ring[2*i+k]=ring[2*j+k];ring[2*j+k]=temp;
                }
            }
            area2=-area2;
        }
        for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
            int i2=(i+1)%n,j2=(j+1)%n;
            if(i==j||i==j2||i2==j)continue; // adjacent edges share one endpoint
            if(intersects(ring,i,i2,j,j2,eps))
                throw new IllegalArgumentException("Self-intersecting polygon");
        }
        int[] cap=triangulate(ring,n,eps);
        double lo=Math.min(baseZ,baseZ+height),hi=Math.max(baseZ,baseZ+height);
        double[] xyz=new double[6*n];
        for(int i=0;i<n;i++){
            xyz[i*3]=xyz[(n+i)*3]=ring[2*i];
            xyz[i*3+1]=xyz[(n+i)*3+1]=ring[2*i+1];
            xyz[i*3+2]=lo;xyz[(n+i)*3+2]=hi;
        }
        int[] tris=new int[(cap.length*2)+(n*6)];
        int out=0;
        for(int i=0;i<cap.length;i+=3){
            // Bottom normal -Z, top normal +Z.
            tris[out++]=cap[i];tris[out++]=cap[i+2];tris[out++]=cap[i+1];
            tris[out++]=n+cap[i];tris[out++]=n+cap[i+1];tris[out++]=n+cap[i+2];
        }
        for(int i=0;i<n;i++){
            int j=(i+1)%n;
            tris[out++]=i;tris[out++]=j;tris[out++]=n+j;
            tris[out++]=i;tris[out++]=n+j;tris[out++]=n+i;
        }
        Solid solid=new Solid(xyz,tris,0.5*area2*(hi-lo),n);
        if(!solid.isWatertight())throw new IllegalArgumentException("Non-manifold generated shell");
        // A signed surface volume independently checks face normals.
        double sixVolume=0,ox=xyz[0],oy=xyz[1],oz=xyz[2];
        for(int i=0;i<tris.length;i+=3){
            int a=tris[i]*3,b=tris[i+1]*3,c=tris[i+2]*3;
            double ax=xyz[a]-ox,ay=xyz[a+1]-oy,az=xyz[a+2]-oz;
            double bx=xyz[b]-ox,by=xyz[b+1]-oy,bz=xyz[b+2]-oz;
            double cx=xyz[c]-ox,cy=xyz[c+1]-oy,cz=xyz[c+2]-oz;
            sixVolume+=ax*(by*cz-bz*cy)+ay*(bz*cx-bx*cz)+az*(bx*cy-by*cx);
        }
        double delta=Math.abs(sixVolume/6-solid.volume);
        if(sixVolume<=0||delta>Math.max(1e-8,solid.volume*1e-8))
            throw new IllegalArgumentException("Invalid shell orientation or volume");
        return solid;
    }

    private static boolean bounded(double v){return Double.isFinite(v)&&Math.abs(v)<=1e9;}
    private static double cross(double[] p,int a,int b,int c){
        double x=p[2*b]-p[2*a],y=p[2*b+1]-p[2*a+1];
        return x*(p[2*c+1]-p[2*a+1])-y*(p[2*c]-p[2*a]);
    }
    private static boolean between(double a,double b,double x,double eps) {
        return x>=Math.min(a,b)-eps && x<=Math.max(a,b)+eps;
    }
    private static boolean onSegment(double[] p,int a,int b,int x,double eps){
        return Math.abs(cross(p,a,b,x))<=eps &&
                between(p[2*a],p[2*b],p[2*x],eps) &&
                between(p[2*a+1],p[2*b+1],p[2*x+1],eps);
    }
    private static boolean intersects(double[] p,int a,int b,int c,int d,double eps){
        double abC=cross(p,a,b,c),abD=cross(p,a,b,d);
        double cdA=cross(p,c,d,a),cdB=cross(p,c,d,b);
        if((abC>eps&&abD< -eps||abC< -eps&&abD>eps) &&
           (cdA>eps&&cdB< -eps||cdA< -eps&&cdB>eps))return true;
        return onSegment(p,a,b,c,eps)||onSegment(p,a,b,d,eps)||
                onSegment(p,c,d,a,eps)||onSegment(p,c,d,b,eps);
    }
    private static int[] triangulate(double[] p,int n,double eps){
        ArrayList<Integer> ring=new ArrayList<>(n);
        for(int i=0;i<n;i++)ring.add(i);
        int[] result=new int[(n-2)*3];int used=0;
        while(ring.size()>3){
            boolean found=false;
            int size=ring.size();
            for(int k=0;k<size;k++){
                int a=ring.get((k+size-1)%size),b=ring.get(k),c=ring.get((k+1)%size);
                if(cross(p,a,b,c)<=eps)continue;
                boolean contains=false;
                for(int t=0;t<size;t++){
                    int v=ring.get(t);
                    if(v==a||v==b||v==c)continue;
                    if(cross(p,a,b,v)>=-eps&&cross(p,b,c,v)>=-eps&&
                            cross(p,c,a,v)>=-eps){contains=true;break;}
                }
                if(contains)continue;
                result[used++]=a;result[used++]=b;result[used++]=c;
                ring.remove(k);found=true;break;
            }
            if(!found)throw new IllegalArgumentException("Polygon triangulation failed");
        }
        result[used++]=ring.get(0);result[used++]=ring.get(1);result[used++]=ring.get(2);
        return result;
    }
}
