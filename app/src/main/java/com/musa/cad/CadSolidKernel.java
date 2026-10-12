package com.musa.cad;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

/**
 * Deterministic triangular CAD mesh operations. This is NOT an ACIS B-Rep
 * writer: EXTRUDE/REVOLVE and exact orthogonal BOX booleans export 3DFACE DXF.
 * Input geometry is not silently modified.
 */
public final class CadSolidKernel {
    private static final int MAX_POINTS=500_000,MAX_FACES=1_000_000;
    private static final double EPS=1e-9;
    private CadSolidKernel(){}

    public static final class Mesh {
        public final float[] xyz;
        public final int[] triangles;
        private Mesh(float[] xyz,int[] triangles)throws IOException{
            if(xyz.length==0||xyz.length%3!=0||triangles.length==0||triangles.length%3!=0
                ||xyz.length/3>MAX_POINTS||triangles.length/3>MAX_FACES)
                throw new IOException("3B ağ boyutları geçersiz veya güvenlik sınırını aşıyor");
            for(float v:xyz)if(!Float.isFinite(v))throw new IOException("Geçersiz 3B koordinat");
            for(int i:triangles)if(i<0||i>=xyz.length/3)throw new IOException("Yüzey köşe indeksi geçersiz");
            this.xyz=xyz.clone();this.triangles=triangles.clone();
        }
        public int vertexCount(){return xyz.length/3;}
        public int faceCount(){return triangles.length/3;}
    }
    private static final class Builder {
        private final ArrayList<Float> xyz=new ArrayList<>();
        private final ArrayList<Integer> tri=new ArrayList<>();
        private final HashMap<String,Integer> shared=new HashMap<>();
        int vertex(double x,double y,double z)throws IOException{
            if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z)
                ||Math.max(Math.abs(x),Math.max(Math.abs(y),Math.abs(z)))>1e9)
                throw new IOException("3B koordinat güvenlik sınırı aşıldı");
            float fx=(float)x,fy=(float)y,fz=(float)z;
            // Android mesh is float-based; never silently collapse engineering
            // coordinates whose numeric error exceeds 0.001 drawing unit.
            if(Math.abs((double)fx-x)>0.001d||Math.abs((double)fy-y)>0.001d
                ||Math.abs((double)fz-z)>0.001d)
                throw new IOException("3B koordinat float hassasiyetini aşıyor; orijin yakınında modelleyin");
            String key=Double.toHexString(x)+","+Double.toHexString(y)+","+Double.toHexString(z);
            Integer previous=shared.get(key);if(previous!=null)return previous;
            if(xyz.size()/3>=MAX_POINTS)throw new IOException("3B köşe sınırı aşıldı");
            int index=xyz.size()/3;
            xyz.add(fx);xyz.add(fy);xyz.add(fz);
            shared.put(key,index);
            return index;
        }
        void triangle(int a,int b,int c)throws IOException{
            if(a==b||a==c||b==c)return;
            if(tri.size()/3>=MAX_FACES)throw new IOException("3B yüzey sınırı aşıldı");
            tri.add(a);tri.add(b);tri.add(c);
        }
        void quad(int a,int b,int c,int d)throws IOException{
            triangle(a,b,c);triangle(a,c,d);
        }
        Mesh build()throws IOException{
            float[] p=new float[xyz.size()];for(int i=0;i<p.length;i++)p[i]=xyz.get(i);
            int[] t=new int[tri.size()];for(int i=0;i<t.length;i++)t[i]=tri.get(i);
            return new Mesh(p,t);
        }
    }

    private static void checkPolygon(double[] xy)throws IOException{
        if(xy==null||xy.length<6||xy.length%2!=0||xy.length/2>10000)
            throw new IOException("Kapalı 2B profil en az üç noktadan oluşmalıdır");
        int n=xy.length/2;
        for(double v:xy)if(!Double.isFinite(v)||Math.abs(v)>1e9)
            throw new IOException("Geçersiz profil koordinatı");
        if(Math.abs(area(xy))<EPS)throw new IOException("Profil alanı sıfır");
        for(int i=0;i<n;i++){
            int in=(i+1)%n;
            if(Math.hypot(xy[i*2]-xy[in*2],xy[i*2+1]-xy[in*2+1])<EPS)
                throw new IOException("Profil tekrarlı/çok yakın ardışık nokta içeriyor");
            for(int j=i+1;j<n;j++){
                int jn=(j+1)%n;
                if(i==j||in==j||jn==i)continue;
                if(crosses(xy[i*2],xy[i*2+1],xy[in*2],xy[in*2+1],
                    xy[j*2],xy[j*2+1],xy[jn*2],xy[jn*2+1]))
                    throw new IOException("Profil kendi kendini kesiyor");
            }
        }
    }
    private static double orient(double ax,double ay,double bx,double by,double cx,double cy){
        return (bx-ax)*(cy-ay)-(by-ay)*(cx-ax);
    }
    private static boolean crosses(double ax,double ay,double bx,double by,
                                    double cx,double cy,double dx,double dy){
        double a=orient(ax,ay,bx,by,cx,cy),b=orient(ax,ay,bx,by,dx,dy),
               c=orient(cx,cy,dx,dy,ax,ay),d=orient(cx,cy,dx,dy,bx,by);
        return (a>EPS&&b< -EPS||a< -EPS&&b>EPS)
            &&(c>EPS&&d< -EPS||c< -EPS&&d>EPS);
    }
    private static double area(double[] p){
        double s=0;for(int i=0,n=p.length/2;i<n;i++){
            int j=(i+1)%n;s+=p[i*2]*p[j*2+1]-p[j*2]*p[i*2+1];
        }return s*.5;
    }
    /** Ear clipping, in the input vertex indexing order, for a simple polygon. */
    private static int[] caps(double[] profile)throws IOException{
        checkPolygon(profile);
        int n=profile.length/2;
        ArrayList<Integer> available=new ArrayList<>();
        boolean ccw=area(profile)>0;
        for(int i=0;i<n;i++)available.add(ccw?i:n-1-i);
        ArrayList<Integer> faces=new ArrayList<>();
        int guard=0;
        while(available.size()>3){
            boolean found=false;int m=available.size();
            for(int k=0;k<m;k++){
                int a=available.get((k+m-1)%m),b=available.get(k),c=available.get((k+1)%m);
                double ax=profile[2*a],ay=profile[2*a+1],bx=profile[2*b],by=profile[2*b+1],cx=profile[2*c],cy=profile[2*c+1];
                if(orient(ax,ay,bx,by,cx,cy)<=EPS)continue;
                boolean occupied=false;
                for(int idx:available){
                    if(idx==a||idx==b||idx==c)continue;
                    double px=profile[idx*2],py=profile[idx*2+1];
                    if(orient(ax,ay,bx,by,px,py)>=-EPS
                        &&orient(bx,by,cx,cy,px,py)>=-EPS
                        &&orient(cx,cy,ax,ay,px,py)>=-EPS){occupied=true;break;}
                }
                if(occupied)continue;
                faces.add(a);faces.add(b);faces.add(c);available.remove(k);found=true;break;
            }
            if(!found||++guard>n*n)throw new IOException("Profil kulak kesme ile güvenle üçgenlenemedi");
        }
        for(int value:available)faces.add(value);
        int[] result=new int[faces.size()];
        for(int i=0;i<result.length;i++)result[i]=faces.get(i);
        return result; // triangles have CCW winding
    }

    public static Mesh extrude(double[] profile,double bottom,double height)throws IOException{
        if(!Double.isFinite(bottom)||!Double.isFinite(height)||Math.abs(height)<EPS)
            throw new IOException("EXTRUDE yüksekliği sıfır veya geçersiz");
        int[] triangles=caps(profile);
        int n=profile.length/2;boolean ccw=area(profile)>0,up=height>0;
        Builder b=new Builder();int[] lo=new int[n],hi=new int[n];
        for(int i=0;i<n;i++){lo[i]=b.vertex(profile[2*i],profile[2*i+1],bottom);
            hi[i]=b.vertex(profile[2*i],profile[2*i+1],bottom+height);}
        for(int i=0;i<triangles.length;i+=3){
            int a=triangles[i],c=triangles[i+1],d=triangles[i+2];
            if(up){b.triangle(lo[a],lo[d],lo[c]);b.triangle(hi[a],hi[c],hi[d]);}
            else {b.triangle(hi[a],hi[d],hi[c]);b.triangle(lo[a],lo[c],lo[d]);}
        }
        for(int i=0;i<n;i++){
            int j=(i+1)%n;
            if(ccw==up)b.quad(lo[i],lo[j],hi[j],hi[i]);
            else b.quad(lo[j],lo[i],hi[i],hi[j]);
        }
        return b.build();
    }

    /**
     * Rotate a closed (radius,z) profile about the global Z axis. Radius >0
     * avoids axis singularities. Full sweep yields a closed toroidal/cylindrical
     * shell; a partial sweep gains triangulated end caps.
     */
    public static Mesh revolve(double[] profile,int segments,double sweepDegrees)throws IOException{
        int[] capTriangles=caps(profile);
        if(segments<8||segments>720||!Double.isFinite(sweepDegrees)
            ||sweepDegrees<=0||sweepDegrees>360)
            throw new IOException("REVOLVE segment/açı parametreleri geçersiz");
        for(int i=0;i<profile.length;i+=2)
            if(profile[i]<=EPS)throw new IOException("REVOLVE profil yarıçapı sıfır olamaz (bu aşamada)");
        int n=profile.length/2;boolean complete=Math.abs(sweepDegrees-360d)<1e-8;
        if((long)n*(segments+(complete?0:1))>MAX_POINTS)throw new IOException("REVOLVE ağ köşe sınırı aşıldı");
        boolean ccw=area(profile)>0;
        int rings=complete?segments:segments+1;
        Builder b=new Builder();int[][] ids=new int[rings][n];
        for(int j=0;j<rings;j++){
            double t=Math.toRadians(sweepDegrees*j/segments);
            for(int i=0;i<n;i++)ids[j][i]=b.vertex(profile[2*i]*Math.cos(t),profile[2*i]*Math.sin(t),profile[2*i+1]);
        }
        for(int j=0;j<segments;j++){
            int next=(j+1)%rings;
            for(int i=0;i<n;i++){
                int ni=(i+1)%n,a=ids[j][i],d=ids[next][i],c=ids[next][ni],e=ids[j][ni];
                if(ccw)b.quad(a,d,c,e);
                else b.quad(e,c,d,a);
            }
        }
        if(!complete){
            for(int j=0;j<capTriangles.length;j+=3){
                int a=capTriangles[j],d=capTriangles[j+1],e=capTriangles[j+2];
                if(ccw){b.triangle(ids[0][a],ids[0][d],ids[0][e]);
                    b.triangle(ids[rings-1][a],ids[rings-1][e],ids[rings-1][d]);}
                else {b.triangle(ids[0][a],ids[0][e],ids[0][d]);
                    b.triangle(ids[rings-1][a],ids[rings-1][d],ids[rings-1][e]);}
            }
        }
        return b.build();
    }

    /**
     * LOFT between 2+ ordered XY contours with one-to-one vertex correspondence.
     * Contours must have the same point count and winding, with ascending Z.
     * Rejects insufficient/degenerate profiles; complex intersecting lofts
     * should be checked by engineering tools before manufacture.
     */
    public static Mesh loft(double[][] contours,double[] heights)throws IOException{
        if(contours==null||heights==null||contours.length<2
            ||contours.length!=heights.length||contours.length>256)
            throw new IOException("LOFT için en az iki kesit ve her kesitin kotu gerekir");
        int n=contours[0].length/2;
        if((long)n*contours.length>MAX_POINTS)throw new IOException("LOFT model köşe sınırı aşıldı");
        boolean ccw=area(contours[0])>0d;
        for(int k=0;k<contours.length;k++){
            checkPolygon(contours[k]);
            if(contours[k].length/2!=n||ccw!=(area(contours[k])>0d))
                throw new IOException("LOFT kesitlerin nokta sayısı/yönü eşleşmiyor");
            if(!Double.isFinite(heights[k])||
                (k>0&&heights[k]<=heights[k-1]))
                throw new IOException("LOFT kotları artan sırada olmalıdır");
        }
        // Distinct triangulated end-caps, not a fake bounding-box extrusion.
        int[] bottomFaces=caps(contours[0]),topFaces=caps(contours[contours.length-1]);
        Builder b=new Builder();
        int[][] rings=new int[contours.length][n];
        for(int k=0;k<contours.length;k++)for(int i=0;i<n;i++)
            rings[k][i]=b.vertex(contours[k][i*2],contours[k][i*2+1],heights[k]);
        for(int i=0;i<bottomFaces.length;i+=3){
            int a=bottomFaces[i],d=bottomFaces[i+1],e=bottomFaces[i+2];
            b.triangle(rings[0][a],rings[0][e],rings[0][d]);
        }
        int last=rings.length-1;
        for(int i=0;i<topFaces.length;i+=3){
            int a=topFaces[i],d=topFaces[i+1],e=topFaces[i+2];
            b.triangle(rings[last][a],rings[last][d],rings[last][e]);
        }
        for(int k=0;k+1<rings.length;k++)for(int i=0;i<n;i++){
            int j=(i+1)%n;
            if(ccw)b.quad(rings[k][i],rings[k][j],rings[k+1][j],rings[k+1][i]);
            else b.quad(rings[k][j],rings[k][i],rings[k+1][i],rings[k+1][j]);
        }
        return b.build();
    }

    /**
     * SWEEP of an unchanged XY cross-section along a 3D polyline with strictly
     * increasing Z. Translation-only: section remains parallel to world XY,
     * never claims a path-tangent frame or closed general NURBS sweep.
     */
    public static Mesh sweepXY(double[] profile,double[] pathXYZ)throws IOException{
        checkPolygon(profile);
        if(pathXYZ==null||pathXYZ.length<6||pathXYZ.length%3!=0
            ||pathXYZ.length/3>256)
            throw new IOException("SWEEP için en az iki XYZ yol noktası gereklidir");
        int n=profile.length/2,rings=pathXYZ.length/3;
        double[][] shapes=new double[rings][profile.length];
        double[] heights=new double[rings];
        for(int k=0;k<rings;k++){
            double dx=pathXYZ[3*k],dy=pathXYZ[3*k+1],z=pathXYZ[3*k+2];
            if(!Double.isFinite(dx)||!Double.isFinite(dy)||!Double.isFinite(z))
                throw new IOException("SWEEP yol koordinatı geçersiz");
            heights[k]=z;
            for(int i=0;i<n;i++){
                shapes[k][2*i]=profile[2*i]+dx;
                shapes[k][2*i+1]=profile[2*i+1]+dy;
            }
        }
        return loft(shapes,heights);
    }

    public static final class Box {
        public final double[] min,max;
        public Box(double x0,double y0,double z0,double x1,double y1,double z1)throws IOException{
            double[] a={x0,y0,z0},d={x1,y1,z1};
            for(int i=0;i<3;i++){
                if(!Double.isFinite(a[i])||!Double.isFinite(d[i])
                    ||a[i]>=d[i]||Math.max(Math.abs(a[i]),Math.abs(d[i]))>1e9)
                    throw new IOException("Kutu koordinatları geçersiz");
            }
            min=a;max=d;
        }
        boolean contains(double x,double y,double z){
            return x>min[0]&&x<max[0]&&y>min[1]&&y<max[1]&&z>min[2]&&z<max[2];
        }
    }
    public enum BooleanOp { UNION, INTERSECTION, DIFFERENCE }

    /**
     * Exact orthogonal box CSG (not generic arbitrary ACIS Boolean).
     * Split at both boxes' face coordinates, emit only occupied/exposed cells.
     */
    public static Mesh boxBoolean(Box a,Box b,BooleanOp op)throws IOException{
        if(a==null||b==null||op==null)throw new IOException("BOOLEAN kutuları eksik");
        double[][] ticks=new double[3][];
        for(int axis=0;axis<3;axis++){
            double[] xs={a.min[axis],a.max[axis],b.min[axis],b.max[axis]};
            Arrays.sort(xs);
            ArrayList<Double> unique=new ArrayList<>();
            for(double x:xs)if(unique.isEmpty()||x>unique.get(unique.size()-1))unique.add(x);
            ticks[axis]=new double[unique.size()];
            for(int i=0;i<unique.size();i++)ticks[axis][i]=unique.get(i);
        }
        int nx=ticks[0].length-1,ny=ticks[1].length-1,nz=ticks[2].length-1;
        boolean[][][] occupied=new boolean[nx][ny][nz];
        for(int i=0;i<nx;i++)for(int j=0;j<ny;j++)for(int k=0;k<nz;k++){
            double x=(ticks[0][i]+ticks[0][i+1])*.5,y=(ticks[1][j]+ticks[1][j+1])*.5,z=(ticks[2][k]+ticks[2][k+1])*.5;
            boolean av=a.contains(x,y,z),bv=b.contains(x,y,z);
            occupied[i][j][k]=op==BooleanOp.UNION?av||bv:op==BooleanOp.INTERSECTION?av&&bv:av&&!bv;
        }
        Builder build=new Builder();
        for(int i=0;i<nx;i++)for(int j=0;j<ny;j++)for(int k=0;k<nz;k++){
            if(!occupied[i][j][k])continue;
            double x=ticks[0][i],X=ticks[0][i+1],y=ticks[1][j],Y=ticks[1][j+1],z=ticks[2][k],Z=ticks[2][k+1];
            int p000=build.vertex(x,y,z),p001=build.vertex(x,y,Z),p010=build.vertex(x,Y,z),p011=build.vertex(x,Y,Z);
            int p100=build.vertex(X,y,z),p101=build.vertex(X,y,Z),p110=build.vertex(X,Y,z),p111=build.vertex(X,Y,Z);
            if(i==0||!occupied[i-1][j][k])build.quad(p000,p001,p011,p010);
            if(i==nx-1||!occupied[i+1][j][k])build.quad(p100,p110,p111,p101);
            if(j==0||!occupied[i][j-1][k])build.quad(p000,p100,p101,p001);
            if(j==ny-1||!occupied[i][j+1][k])build.quad(p010,p011,p111,p110);
            if(k==0||!occupied[i][j][k-1])build.quad(p000,p010,p110,p100);
            if(k==nz-1||!occupied[i][j][k+1])build.quad(p001,p101,p111,p011);
        }
        return build.build(); // empty intersection intentionally reports no solid
    }
    public static Mesh box(Box box)throws IOException{
        if(box==null)throw new IOException("Kutu bulunamadı");
        // A full box is just an extruded rectangle; preserve exact bounds.
        return extrude(new double[]{box.min[0],box.min[1],box.max[0],box.min[1],
            box.max[0],box.max[1],box.min[0],box.max[1]},
            box.min[2],box.max[2]-box.min[2]);
    }

    /** Transform any generated triangular model from an arbitrary UCS into WCS. */
    public static Mesh inWorld(Mesh mesh,CadUcs3d ucs)throws IOException{
        if(mesh==null||ucs==null)throw new IOException("UCS modeli veya koordinat sistemi bulunamadı");
        float[] moved=new float[mesh.xyz.length];
        for(int i=0;i<mesh.xyz.length;i+=3){
            double[] p=ucs.toWorld(mesh.xyz[i],mesh.xyz[i+1],mesh.xyz[i+2]);
            for(int j=0;j<3;j++){
                if(!Double.isFinite(p[j])||Math.abs(p[j])>1e9)
                    throw new IOException("UCS dönüşüm sonucu koordinat sınırı aşıldı");
                moved[i+j]=(float)p[j];
            }
        }
        return new Mesh(moved,mesh.triangles);
    }

    public static void writeDxf(Mesh mesh,OutputStream output)throws IOException{
        if(mesh==null||output==null)throw new IOException("3B DXF hedefi eksik");
        try(BufferedWriter out=new BufferedWriter(new OutputStreamWriter(output,StandardCharsets.US_ASCII),65536)){
            out.write("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n");
            out.write("0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"+
                "0\nLAYER\n2\nMUSACAD_SOLID_MESH\n70\n0\n62\n7\n6\nCONTINUOUS\n"+
                "0\nENDTAB\n0\nENDSEC\n");
            out.write("0\nSECTION\n2\nENTITIES\n");
            for(int f=0;f<mesh.triangles.length;f+=3){
                FileTransfer.checkCancelled();
                out.write("0\n3DFACE\n8\nMUSACAD_SOLID_MESH\n");
                for(int i=0;i<4;i++){
                    int vertex=mesh.triangles[f+(i==3?2:i)]*3;
                    pair(out,10+i,mesh.xyz[vertex]);pair(out,20+i,mesh.xyz[vertex+1]);pair(out,30+i,mesh.xyz[vertex+2]);
                }
            }
            out.write("0\nENDSEC\n0\nEOF\n");out.flush();
        }
    }
    private static void pair(BufferedWriter out,int code,float value)throws IOException{
        out.write(Integer.toString(code));out.write('\n');out.write(Float.toString(value));out.write('\n');
    }
}
