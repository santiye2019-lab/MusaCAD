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
    // Triangulation and self-intersection screening are O(n²).
    // Keep this developer-only primitive bounded on mobile processors.
    private static final int MAX_RING = 512;

    public static final class Solid {
        /** XYZ interleaved: first the low ring, then the high ring. */
        public final double[] xyz;
        /** Outward-oriented triangle vertex indices. */
        public final int[] triangles;
        public final double volume;
        public final int ringVertices;
        /** Planar polygon perimeter in native drawing units. */
        public final double perimeter;
        /** 2*profile area + perimeter*extrusion height (no internal holes). */
        public final double surfaceArea;
        /** Exact centroid of this uniform-density polygonal prism in WCS. */
        public final double centroidX,centroidY,centroidZ;

        private Solid(double[] xyz,int[] triangles,double volume,int ringVertices,
                      double perimeter,double surfaceArea,
                      double centroidX,double centroidY,double centroidZ){
            this.xyz=xyz;this.triangles=triangles;this.volume=volume;
            this.ringVertices=ringVertices;this.perimeter=perimeter;
            this.surfaceArea=surfaceArea;
            this.centroidX=centroidX;this.centroidY=centroidY;this.centroidZ=centroidZ;
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

        private static void tag(Writer out,int group,String value)throws IOException{
            out.write(Integer.toString(group));out.write("\n");out.write(value);out.write("\n");
        }
    }

    /**
     * Straight-path SWEEP of a validated XY polygon by a 3D displacement.
     * Supports an oblique extrusion vector with nonzero Z component.
     *
     * The top/bottom caps are translated copies of the same planar profile,
     * connected by planar parallelograms. This is exact polygonal geometry,
     * but NOT ACIS 3DSOLID/SWEEP, profile twist, curved-path sweeping,
     * spline surfaces or a native binary DWG writer.
     */
    public static Solid sweepLinear(double[] ringXY,double baseZ,
                                    double displacementX,double displacementY,
                                    double displacementZ){
        if(!bounded(displacementX)||!bounded(displacementY)||
           !bounded(displacementZ)||Math.abs(displacementZ)<=1e-9d)
            throw new IllegalArgumentException("SWEEP displacement invalid or coplanar");
        // Existing EXTRUDE first validates winding, triangulation, edge
        // incidence, polygon self-intersection, and signed closed volume.
        Solid axial=extrude(ringXY,baseZ,displacementZ);
        final int n=axial.ringVertices;
        double[] vertices=Arrays.copyOf(axial.xyz,axial.xyz.length);
        // extrude() always stores the lower-Z ring before the higher one.
        // Move the destination ring, not simply the higher ring: the path
        // may point downwards and still needs the requested XY displacement.
        int target=displacementZ>0d?n:0;
        for(int i=0;i<n;i++){
            int at=3*(target+i);
            double x=vertices[at]+displacementX;
            double y=vertices[at+1]+displacementY;
            if(!bounded(x)||!bounded(y))
                throw new IllegalArgumentException("SWEEP displaced vertex out of bounds");
            vertices[at]=x;vertices[at+1]=y;
        }

        // Surface is both unchanged polygonal end caps plus the exact
        // parallelogram side area. Each original XY segment u spans the
        // displacement v; the side is ||u cross v||. This catches shear
        // rather than reporting the incorrect axial perimeter*height.
        double caps=2d*(axial.volume/Math.abs(displacementZ));
        double side=0d,sideCorrection=0d;
        for(int i=0;i<n;i++){
            int j=(i+1)%n;
            double ux=axial.xyz[3*j]-axial.xyz[3*i];
            double uy=axial.xyz[3*j+1]-axial.xyz[3*i+1];
            double area=Math.hypot(Math.hypot(uy*displacementZ,
                                             ux*displacementZ),
                                   ux*displacementY-uy*displacementX);
            if(!Double.isFinite(area))
                throw new IllegalArgumentException("SWEEP side area overflow");
            double next=side+area;
            if(Math.abs(side)>=Math.abs(area))sideCorrection+=(side-next)+area;
            else sideCorrection+=(area-next)+side;
            side=next;
        }
        double surface=caps+side+sideCorrection;
        double cx=axial.centroidX+displacementX*0.5d;
        double cy=axial.centroidY+displacementY*0.5d;
        if(!Double.isFinite(surface)||!(surface>0d)||
           !bounded(cx)||!bounded(cy))
            throw new IllegalArgumentException("SWEEP analytic properties invalid");

        Solid result=new Solid(vertices,Arrays.copyOf(axial.triangles,axial.triangles.length),
                                axial.volume,n,axial.perimeter,surface,
                                cx,cy,axial.centroidZ);
        if(!result.isWatertight())
            throw new IllegalArgumentException("SWEEP nonmanifold mesh");

        // Independent signed-tetrahedron check: oblique shearing must not
        // change volume or turn an oriented shell inside out.
        double sixVolume=0d;
        double ox=vertices[0],oy=vertices[1],oz=vertices[2];
        for(int t=0;t<result.triangles.length;t+=3){
            int a=result.triangles[t]*3,b=result.triangles[t+1]*3,
                c=result.triangles[t+2]*3;
            double ax=vertices[a]-ox,ay=vertices[a+1]-oy,az=vertices[a+2]-oz;
            double bx=vertices[b]-ox,by=vertices[b+1]-oy,bz=vertices[b+2]-oz;
            double cx0=vertices[c]-ox,cy0=vertices[c+1]-oy,cz0=vertices[c+2]-oz;
            sixVolume+=ax*(by*cz0-bz*cy0)+
                       ay*(bz*cx0-bx*cz0)+az*(bx*cy0-by*cx0);
        }
        double measured=sixVolume/6d;
        if(!(measured>0d)||!Double.isFinite(measured)||
           Math.abs(measured-result.volume)>Math.max(1e-8,result.volume*1e-8))
            throw new IllegalArgumentException("SWEEP shell signed volume mismatch");
        return result;
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
        // Analytic mass properties of a constant-section polygonal prism.
        // Use coordinates relative to the WCS bounding corner to avoid
        // cancellation when georeferenced drawings sit far from (0,0,0).
        double centroidMomentX=0d,centroidMomentY=0d;
        double checkArea2=0d,perimeter=0d;
        for(int i=0;i<n;i++){
            int next=(i+1)%n;
            double x=ring[2*i]-minX,y=ring[2*i+1]-minY;
            double xNext=ring[2*next]-minX,yNext=ring[2*next+1]-minY;
            double cross=x*yNext-xNext*y;
            checkArea2+=cross;
            centroidMomentX+=(x+xNext)*cross;
            centroidMomentY+=(y+yNext)*cross;
            perimeter+=Math.hypot(ring[2*next]-ring[2*i],
                                  ring[2*next+1]-ring[2*i+1]);
        }
        if(!(checkArea2>eps)||Math.abs(checkArea2-area2)>Math.max(eps,area2*1e-10))
            throw new IllegalArgumentException("Inconsistent extrusion profile area");
        double centroidX=minX+centroidMomentX/(3d*checkArea2);
        double centroidY=minY+centroidMomentY/(3d*checkArea2);
        double centroidZ=lo+(hi-lo)*0.5d;
        double totalSurface=area2+perimeter*(hi-lo);
        if(!Double.isFinite(centroidX)||!Double.isFinite(centroidY)||
           !Double.isFinite(centroidZ)||!Double.isFinite(perimeter)||
           !Double.isFinite(totalSurface)||!(perimeter>0)||!(totalSurface>0))
            throw new IllegalArgumentException("Extrusion mass properties overflow");
        Solid solid=new Solid(xyz,tris,0.5*area2*(hi-lo),n,perimeter,
                              totalSurface,centroidX,centroidY,centroidZ);
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
