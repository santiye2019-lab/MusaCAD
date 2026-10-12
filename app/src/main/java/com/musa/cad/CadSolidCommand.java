package com.musa.cad;

import java.io.IOException;
import java.util.Locale;

/** Locale-independent, explicitly scoped 3D modeling commands. */
public final class CadSolidCommand {
    private CadSolidCommand(){}
    private static double[] numbers(String s,int expected)throws IOException{
        String[] values=s.trim().split("\\s*,\\s*",-1);
        if(expected>0&&values.length!=expected)
            throw new IOException("Beklenen koordinat sayısı: "+expected);
        double[] out=new double[values.length];
        for(int i=0;i<values.length;i++){
            try{out[i]=Double.parseDouble(values[i].trim());}
            catch(NumberFormatException e){throw new IOException("Geçersiz sayısal koordinat: "+values[i],e);}
            if(!Double.isFinite(out[i]))throw new IOException("Sonsuz veya geçersiz koordinat");
        }
        return out;
    }
    private static double[] polygon(String value)throws IOException{
        String[] pairs=value.trim().split("\\s*;\\s*");
        if(pairs.length<3||pairs.length>10000)throw new IOException("En az 3 profil noktası gerekli");
        double[] all=new double[pairs.length*2];
        for(int i=0;i<pairs.length;i++){
            double[] xy=numbers(pairs[i],2);
            all[2*i]=xy[0];all[2*i+1]=xy[1];
        }
        return all;
    }
    private static CadSolidKernel.Box box(String spec)throws IOException{
        double[] six=numbers(spec,6);
        return new CadSolidKernel.Box(six[0],six[1],six[2],six[3],six[4],six[5]);
    }
    public static CadSolidKernel.Mesh applyUcs(CadSolidKernel.Mesh model,String spec)throws IOException{
        if(model==null)throw new IOException("Önce BOX / EXTRUDE / REVOLVE / BOOLEAN ile model oluşturun");
        String[] parts=spec==null?new String[0]:spec.trim().split("\\s*\\|\\s*",-1);
        if(parts.length!=3)throw new IOException("UCS: orijinX,Y,Z|XeksenX,Y,Z|YeksenX,Y,Z");
        return CadSolidKernel.inWorld(model,
            CadUcs3d.of(numbers(parts[0],3),numbers(parts[1],3),numbers(parts[2],3)));
    }

    public static CadSolidKernel.Mesh execute(String operation,String specification)throws IOException{
        if(operation==null||specification==null)throw new IOException("3B komut boş");
        String[] segments=specification.trim().split("\\s*\\|\\s*",-1);
        switch(operation.trim().toUpperCase(Locale.ROOT)){
            case "BOX":
                if(segments.length!=1)throw new IOException("BOX: x0,y0,z0,x1,y1,z1");
                return CadSolidKernel.box(box(segments[0]));
            case "EXTRUDE":{
                if(segments.length!=2&&segments.length!=3)
                    throw new IOException("EXTRUDE: x,y;...|yükseklik[|altZ]");
                double[] h=numbers(segments[1],1);
                double base=segments.length==3?numbers(segments[2],1)[0]:0;
                return CadSolidKernel.extrude(polygon(segments[0]),base,h[0]);
            }
            case "REVOLVE":{
                if(segments.length!=3)
                    throw new IOException("REVOLVE: yarıçap,z;...|segment|derece");
                double segment=numbers(segments[1],1)[0];
                if(segment!=Math.rint(segment))throw new IOException("Dilim sayısı tam olmalı");
                return CadSolidKernel.revolve(polygon(segments[0]),(int)segment,numbers(segments[2],1)[0]);
            }
            case "LOFT":{
                if(segments.length<4||segments.length%2!=0||segments.length>512)
                    throw new IOException("LOFT: profil1|kot1|profil2|kot2[|profil3|kot3...]");
                int count=segments.length/2;
                double[][] shapes=new double[count][];
                double[] heights=new double[count];
                for(int k=0;k<count;k++){
                    shapes[k]=polygon(segments[2*k]);
                    heights[k]=numbers(segments[2*k+1],1)[0];
                }
                return CadSolidKernel.loft(shapes,heights);
            }
            case "UNION":case "INTERSECTION":case "DIFFERENCE":{
                if(segments.length!=2)
                    throw new IOException("BOOLEAN: ilk kutu|ikinci kutu");
                return CadSolidKernel.boxBoolean(box(segments[0]),box(segments[1]),
                    CadSolidKernel.BooleanOp.valueOf(operation.trim().toUpperCase(Locale.ROOT)));
            }
            default:throw new IOException("Desteklenmeyen 3B modelleme komutu: "+operation);
        }
    }
}
