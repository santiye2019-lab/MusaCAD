package com.musa.cad;

import java.io.IOException;
import java.io.Writer;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;

/**
 * Bounded polygonal full-turn / partial-sector REVOLVE of a simple closed radial/Z section.
 * Every profile radius must be strictly positive (axis-touching profiles
 * require separate apex topology). This is a triangle mesh, NOT an ACIS
 * 3DSOLID or editable native AutoCAD REVOLVE feature.
 */
public final class CadRevolveShell {
    private CadRevolveShell(){}
    private static final int MAX_PROFILE_POINTS=128,MIN_SEGMENTS=12,MAX_SEGMENTS=128;
    private static final double LIMIT=1e7d;

    public static final class Solid {
        public final double[] xyz;
        public final int[] triangles;
        public final int sectionVertices,angularSegments;
        public final double profileArea,sweepDegrees;
        private Solid(double[] xyz,int[] triangles,int sectionVertices,
                      int angularSegments,double profileArea,double sweepDegrees){
            this.xyz=xyz;this.triangles=triangles;
            this.sectionVertices=sectionVertices;this.angularSegments=angularSegments;
            this.profileArea=profileArea;this.sweepDegrees=sweepDegrees;
        }

        /** All oriented triangle edges occur exactly twice with opposite direction. */
        public boolean isWatertight(){
            HashMap<Long,int[]> edges=new HashMap<>();
            for(int t=0;t<triangles.length;t+=3){
                for(int e=0;e<3;e++){
                    int a=triangles[t+e],b=triangles[t+(e+1)%3];
                    if(a<0||b<0||a==b||a>=xyz.length/3||b>=xyz.length/3)return false;
                    int lo=Math.min(a,b),hi=Math.max(a,b);
                    long key=((long)lo<<32)|(hi&0xffffffffL);
                    int[] pair=edges.get(key);
                    if(pair==null)edges.put(key,new int[]{1,a<b?1:-1});
                    else {pair[0]++;pair[1]+=a<b?1:-1;}
                }
            }
            if(edges.isEmpty())return false;
            for(int[] pair:edges.values())if(pair[0]!=2||pair[1]!=0)return false;
            return true;
        }

        /** Writes a new independent DXF 3DFACE mesh; no existing file is mutated. */
        public void writeDxf(Writer target,String layer)throws IOException{
            if(target==null||layer==null||!layer.matches("[A-Za-z0-9_.-]{1,32}"))
                throw new IllegalArgumentException("Invalid REVOLVE DXF output layer");
            tag(target,0,"SECTION");tag(target,2,"HEADER");
            tag(target,9,"$ACADVER");tag(target,1,"AC1015");
            tag(target,0,"ENDSEC");tag(target,0,"SECTION");tag(target,2,"ENTITIES");
            for(int t=0;t<triangles.length;t+=3){
                tag(target,0,"3DFACE");tag(target,8,layer);
                for(int k=0;k<4;k++){
                    int vertex=triangles[t+(k==3?2:k)]*3;
                    tag(target,10+k,Double.toString(xyz[vertex]));
                    tag(target,20+k,Double.toString(xyz[vertex+1]));
                    tag(target,30+k,Double.toString(xyz[vertex+2]));
                }
            }
            tag(target,0,"ENDSEC");tag(target,0,"EOF");
        }

        private static void tag(Writer out,int code,String value)throws IOException{
            out.write(Integer.toString(code));out.write("\n");out.write(value);out.write("\n");
        }
    }

    /**
     * profileRZ = [radius0,z0,radius1,z1,...] describing a closed simple
     * polygon without a repeated final point. Full-turn revolution around
     * the Z axis through axisX,axisY. The returned mesh has no seam slit.
     */
    public static Solid revolve(double[] profileRZ,double axisX,double axisY,int angularSegments){
        return revolve(profileRZ,axisX,axisY,angularSegments,360d);
    }

    /**
     * Positive partial sweeps close BOTH radial end sections with triangulated
     * caps. Only [5,360] degrees; negative sweeps and profiles touching the
     * rotation axis still require a separate geometry implementation.
     */
    public static Solid revolve(double[] profileRZ,double axisX,double axisY,
                                int angularSegments,double sweepDegrees){
        if(!Double.isFinite(sweepDegrees)||sweepDegrees<5d||sweepDegrees>360d)
            throw new IllegalArgumentException("Invalid REVOLVE sweep angle");
        final boolean fullTurn=sweepDegrees==360d;
        if(profileRZ==null||profileRZ.length<6||(profileRZ.length&1)!=0||
           profileRZ.length/2>MAX_PROFILE_POINTS||
           angularSegments<MIN_SEGMENTS||angularSegments>MAX_SEGMENTS||
           !bounded(axisX)||!bounded(axisY))
            throw new IllegalArgumentException("Invalid REVOLVE profile or segments");
        int n=profileRZ.length/2;
        double[] p=Arrays.copyOf(profileRZ,profileRZ.length);
        double minR=Double.POSITIVE_INFINITY,maxR=Double.NEGATIVE_INFINITY;
        double minZ=Double.POSITIVE_INFINITY,maxZ=Double.NEGATIVE_INFINITY;
        for(int i=0;i<n;i++){
            double radius=p[2*i],z=p[2*i+1];
            if(!Double.isFinite(radius)||radius<=1e-6||radius>LIMIT||!bounded(z)||
               !bounded(axisX+radius)||!bounded(axisY+radius)||
               !bounded(axisX-radius)||!bounded(axisY-radius))
                throw new IllegalArgumentException("REVOLVE profile crosses axis or exceeds bounds");
            minR=Math.min(minR,radius);maxR=Math.max(maxR,radius);
            minZ=Math.min(minZ,z);maxZ=Math.max(maxZ,z);
        }
        if(maxR-minR<=1e-6||maxZ-minZ<=1e-6)
            throw new IllegalArgumentException("Collapsed REVOLVE profile");
        double area2=0;
        for(int i=0;i<n;i++){
            int next=(i+1)%n;
            double dr=p[2*next]-p[2*i],dz=p[2*next+1]-p[2*i+1];
            if(Math.hypot(dr,dz)<=1e-6)throw new IllegalArgumentException("Repeated profile corner");
            area2+=(p[2*i]-minR)*(p[2*next+1]-minZ)
                  -(p[2*next]-minR)*(p[2*i+1]-minZ);
        }
        if(Math.abs(area2)<=1e-10*(maxR-minR)*(maxZ-minZ))
            throw new IllegalArgumentException("Zero-area REVOLVE section");
        for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
            int i1=(i+1)%n,j1=(j+1)%n;
            if(i==j||i1==j||j1==i)continue;
            if(touch(p,i,i1,j,j1))
                throw new IllegalArgumentException("Self-intersecting REVOLVE section");
        }
        if(area2<0){
            for(int i=0;i<n/2;i++){
                int j=n-1-i;
                double r=p[2*i],z=p[2*i+1];
                p[2*i]=p[2*j];p[2*i+1]=p[2*j+1];
                p[2*j]=r;p[2*j+1]=z;
            }
            area2=-area2;
        }
        int slices=fullTurn?angularSegments:angularSegments+1;
        double[] xyz=new double[3*n*slices];
        int[] faces=new int[6*n*angularSegments+(fullTurn?0:6*(n-2))];
        for(int i=0;i<n;i++){
            for(int k=0;k<slices;k++){
                double theta=Math.toRadians(sweepDegrees)*k/angularSegments;
                int index=3*(i*slices+k);
                xyz[index]=axisX+p[2*i]*Math.cos(theta);
                xyz[index+1]=axisY+p[2*i]*Math.sin(theta);
                xyz[index+2]=p[2*i+1];
                if(!Double.isFinite(xyz[index])||!Double.isFinite(xyz[index+1])||
                   !bounded(xyz[index])||!bounded(xyz[index+1]))
                    throw new IllegalArgumentException("REVOLVE output overflow");
            }
        }
        int at=0;
        for(int i=0;i<n;i++){
            int next=(i+1)%n;
            for(int k=0;k<angularSegments;k++){
                int k1=fullTurn?(k+1)%slices:k+1;
                int a=i*slices+k,b=i*slices+k1;
                int c=next*slices+k1,d=next*slices+k;
                // For CCW R/Z profiles, increasing theta then increasing
                // section parameter makes the normals face the material exterior.
                faces[at++]=a;faces[at++]=b;faces[at++]=c;
                faces[at++]=a;faces[at++]=c;faces[at++]=d;
            }
        }
        if(!fullTurn){
            // Start section faces the -theta tangent; end section faces
            // +theta. Reversed caps exactly pair all open side perimeter edges.
            int[] cap=triangulateSection(p,n);
            for(int i=0;i<cap.length;i+=3){
                int a=cap[i]*slices,b=cap[i+1]*slices,c=cap[i+2]*slices;
                faces[at++]=a;faces[at++]=b;faces[at++]=c;
                faces[at++]=c+angularSegments;
                faces[at++]=b+angularSegments;
                faces[at++]=a+angularSegments;
            }
        }
        if(at!=faces.length)throw new IllegalArgumentException("REVOLVE triangulation mismatch");
        Solid solid=new Solid(xyz,faces,n,angularSegments,area2/2d,sweepDegrees);
        if(!solid.isWatertight())throw new IllegalArgumentException("Nonmanifold REVOLVE shell");
        return solid;
    }


    /** Bounded ear-clipping of the already validated CCW radial/Z section. */
    private static int[] triangulateSection(double[] p,int n){
        ArrayList<Integer> ring=new ArrayList<>(n);
        for(int i=0;i<n;i++)ring.add(i);
        int[] triangles=new int[3*(n-2)];
        int used=0;
        double minR=Double.POSITIVE_INFINITY,minZ=Double.POSITIVE_INFINITY;
        double maxR=Double.NEGATIVE_INFINITY,maxZ=Double.NEGATIVE_INFINITY;
        for(int i=0;i<n;i++){
            minR=Math.min(minR,p[2*i]);maxR=Math.max(maxR,p[2*i]);
            minZ=Math.min(minZ,p[2*i+1]);maxZ=Math.max(maxZ,p[2*i+1]);
        }
        double epsilon=1e-12*(maxR-minR)*(maxZ-minZ);
        while(ring.size()>3){
            boolean found=false;
            int size=ring.size();
            for(int i=0;i<size;i++){
                int a=ring.get((i+size-1)%size),b=ring.get(i),c=ring.get((i+1)%size);
                if(orient(p,a,b,c)<=epsilon)continue;
                boolean blocked=false;
                for(int j=0;j<size;j++){
                    int v=ring.get(j);
                    if(v==a||v==b||v==c)continue;
                    if(orient(p,a,b,v)>=-epsilon&&orient(p,b,c,v)>=-epsilon&&
                       orient(p,c,a,v)>=-epsilon){blocked=true;break;}
                }
                if(blocked)continue;
                triangles[used++]=a;triangles[used++]=b;triangles[used++]=c;
                ring.remove(i);found=true;break;
            }
            if(!found)throw new IllegalArgumentException("REVOLVE cap triangulation failed");
        }
        triangles[used++]=ring.get(0);
        triangles[used++]=ring.get(1);
        triangles[used++]=ring.get(2);
        return triangles;
    }

    private static boolean bounded(double a){return Double.isFinite(a)&&Math.abs(a)<=LIMIT;}
    private static double orient(double[] p,int a,int b,int c){
        return (p[2*b]-p[2*a])*(p[2*c+1]-p[2*a+1])
              -(p[2*b+1]-p[2*a+1])*(p[2*c]-p[2*a]);
    }
    private static boolean between(double a,double b,double v){
        return v>=Math.min(a,b)&&v<=Math.max(a,b);
    }
    private static boolean on(double[] p,int a,int b,int c){
        return orient(p,a,b,c)==0d&&between(p[2*a],p[2*b],p[2*c])&&
               between(p[2*a+1],p[2*b+1],p[2*c+1]);
    }
    private static boolean touch(double[] p,int a,int b,int c,int d){
        double ac=orient(p,a,b,c),ad=orient(p,a,b,d);
        double ca=orient(p,c,d,a),cb=orient(p,c,d,b);
        if(((ac>0&&ad<0)||(ac<0&&ad>0))&&
           ((ca>0&&cb<0)||(ca<0&&cb>0)))return true;
        return on(p,a,b,c)||on(p,a,b,d)||on(p,c,d,a)||on(p,c,d,b);
    }
}
