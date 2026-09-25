package com.musa.cad;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

/** Writes changed XYZ tags into a new DXF while retaining other drawing records. */
public final class Dxf3dEditor {
    private Dxf3dEditor(){}

    public static void write(File source,OutputStream target,Dxf3dMesh mesh,float[] original)throws IOException{
        if(original.length!=mesh.xyz.length)throw new IOException("3B düzenleme verisi eksik");
        Map<Long,Float> replacements=new HashMap<>();
        for(int i=0;i<original.length;i+=3){
            int slot=mesh.coordinateSlots[i/3],line=mesh.sourceLines[i/3];
            for(int axis=0;axis<3;axis++){
                float value=mesh.xyz[i+axis];
                if(!Float.isFinite(value))throw new IOException("Geçersiz 3B koordinat");
                if(value!=original[i+axis]){
                    if(line<0)throw new IOException("Blok içindeki dönüştürülmüş 3B geometri doğrudan kaydedilemez");
                    replacements.put(key(line,10+axis*10+slot),value);
                }
            }
        }
        if(replacements.isEmpty())throw new IOException("Kaydedilecek 3B değişiklik yok");
        int changed=0,recordLine=-1,line=0;
        try(BufferedInputStream reader=new BufferedInputStream(new FileInputStream(source));
            BufferedOutputStream writer=new BufferedOutputStream(target)){
            byte[] code,value;
            while((code=readLine(reader))!=null){
                value=readLine(reader);if(value==null)throw new IOException("DXF etiket çifti eksik");
                int tag;
                try{tag=Integer.parseInt(new String(code,StandardCharsets.US_ASCII).trim());}catch(NumberFormatException e){throw new IOException("Geçersiz DXF etiketi",e);}
                if(tag==0)recordLine=line;
                Float replacement=replacements.get(key(recordLine,tag));
                writer.write(code);
                if(replacement!=null){
                    writer.write(Float.toString(replacement).getBytes(StandardCharsets.US_ASCII));
                    int n=value.length;if(n>0&&value[n-1]=='\n'){if(n>1&&value[n-2]=='\r')writer.write('\r');writer.write('\n');}
                    changed++;
                }else writer.write(value);
                line+=2;
            }
            if(changed!=replacements.size())throw new IOException("Bazı 3B kaynak koordinatları bulunamadı");
        }
    }
    private static byte[] readLine(BufferedInputStream in)throws IOException{
        ByteArrayOutputStream line=new ByteArrayOutputStream(64);int b;
        while((b=in.read())!=-1){line.write(b);if(b=='\n')break;if(line.size()>1024*1024)throw new IOException("DXF satırı çok uzun");}
        return line.size()==0?null:line.toByteArray();
    }
    private static long key(int line,int code){return ((long)line<<16)|(code&65535L);}
}
