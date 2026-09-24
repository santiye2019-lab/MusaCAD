package com.musa.cad;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.HashMap;
import java.util.Map;

/** Writes changed XYZ tags into a new DXF while retaining other drawing records. */
public final class Dxf3dEditor {
    private Dxf3dEditor(){}

    public static void write(File source,OutputStream target,Dxf3dMesh mesh,float[] original)throws IOException{
        if(original.length!=mesh.xyz.length)throw new IOException("3B düzenleme verisi eksik");
        Map<Long,Float> replacements=new HashMap<>();
        for(int i=0;i<original.length;i+=3){
            if(mesh.sourceLines[i/3]<0)throw new IOException("3B kaynak kaydı bulunamadı");
            int slot=mesh.coordinateSlots[i/3],line=mesh.sourceLines[i/3];
            for(int axis=0;axis<3;axis++){
                float value=mesh.xyz[i+axis];
                if(!Float.isFinite(value))throw new IOException("Geçersiz 3B koordinat");
                if(value!=original[i+axis])replacements.put(key(line,10+axis*10+slot),value);
            }
        }
        if(replacements.isEmpty())throw new IOException("Kaydedilecek 3B değişiklik yok");
        int changed=0,recordLine=-1,line=0;
        try(BufferedReader reader=new BufferedReader(new InputStreamReader(Files.newInputStream(source.toPath()),StandardCharsets.UTF_8));
            BufferedWriter writer=new BufferedWriter(new OutputStreamWriter(target,StandardCharsets.UTF_8))){
            String code,value;
            while((code=reader.readLine())!=null){
                value=reader.readLine();if(value==null)throw new IOException("DXF etiket çifti eksik");
                int tag;
                try{tag=Integer.parseInt(code.trim());}catch(NumberFormatException e){throw new IOException("Geçersiz DXF etiketi",e);}
                if(tag==0)recordLine=line;
                Float replacement=replacements.get(key(recordLine,tag));
                if(replacement!=null){value=Float.toString(replacement);changed++;}
                writer.write(code);writer.newLine();writer.write(value);writer.newLine();line+=2;
            }
            if(changed!=replacements.size())throw new IOException("Bazı 3B kaynak koordinatları bulunamadı");
        }
    }
    private static long key(int line,int code){return ((long)line<<16)|(code&65535L);}
}
