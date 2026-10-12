package com.musa.cad;

import java.io.IOException;
import java.io.OutputStream;
import java.io.BufferedWriter;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

/**
 * Intersect triangular 3D geometry with a horizontal world-Z plane.
 * Returns independent line segments in WCS; coplanar faces are deliberately
 * ignored rather than inventing unreliable filled contours.
 */
public final class CadMeshSection {
    private static final float EPS=1e-6f;
    private static final int MAX_SEGMENTS=1_000_000;
    private CadMeshSection(){}
    public static float[] horizontal(float[] xyz,int[] faces,float planeZ)throws IOException{
        if(xyz==null||faces==null||xyz.length%3!=0||faces.length%3!=0
            ||!Float.isFinite(planeZ))throw new IOException("3B kesit verileri geçersiz");
        ArrayList<Float> points=new ArrayList<>();
        for(int f=0;f<faces.length;f+=3){
            FileTransfer.checkCancelled();
            float[][] corners=new float[3][3];
            for(int j=0;j<3;j++){
                int vertex=faces[f+j];
                if(vertex<0||vertex>=xyz.length/3)throw new IOException("Yüzey köşe indeksi geçersiz");
                for(int axis=0;axis<3;axis++)corners[j][axis]=xyz[vertex*3+axis];
            }
            ArrayList<float[]> cut=new ArrayList<>();
            for(int j=0;j<3;j++){
                float[]a=corners[j],b=corners[(j+1)%3];
                float da=a[2]-planeZ,db=b[2]-planeZ;
                if(Math.abs(da)<EPS&&Math.abs(db)<EPS)continue;
                if((da>EPS&&db>EPS)||(da< -EPS&&db< -EPS))continue;
                float denom=b[2]-a[2];
                if(Math.abs(denom)<EPS)continue;
                float t=(planeZ-a[2])/denom;
                if(t< -EPS||t>1f+EPS)continue;
                float x=a[0]+t*(b[0]-a[0]),y=a[1]+t*(b[1]-a[1]);
                if(!Float.isFinite(x)||!Float.isFinite(y))continue;
                boolean duplicate=false;
                for(float[]p:cut)if(Math.hypot(p[0]-x,p[1]-y)<EPS){duplicate=true;break;}
                if(!duplicate)cut.add(new float[]{x,y,planeZ});
            }
            if(cut.size()!=2)continue;
            if(points.size()/6>=MAX_SEGMENTS)throw new IOException("Kesit çizgisi sınırı aşıldı");
            for(float[]p:cut)for(float v:p)points.add(v);
        }
        float[] result=new float[points.size()];
        for(int i=0;i<result.length;i++)result[i]=points.get(i);
        return result;
    }
    public static void writeDxf(float[] segments,OutputStream target)throws IOException{
        if(segments==null||segments.length%6!=0||segments.length==0||target==null)
            throw new IOException("Kaydedilecek 3B kesit çizgisi bulunamadı");
        try(BufferedWriter out=new BufferedWriter(new OutputStreamWriter(target,StandardCharsets.US_ASCII))){
            out.write("0\nSECTION\n2\nENTITIES\n");
            for(int i=0;i<segments.length;i+=6){
                FileTransfer.checkCancelled();
                out.write("0\nLINE\n8\nMUSACAD_SECTION\n");
                for(int axis=0;axis<3;axis++){
                    out.write(Integer.toString(10+axis*10));out.newLine();
                    out.write(Float.toString(segments[i+axis]));out.newLine();
                    out.write(Integer.toString(11+axis*10));out.newLine();
                    out.write(Float.toString(segments[i+3+axis]));out.newLine();
                }
            }
            out.write("0\nENDSEC\n0\nEOF\n");out.flush();
        }
    }
}
