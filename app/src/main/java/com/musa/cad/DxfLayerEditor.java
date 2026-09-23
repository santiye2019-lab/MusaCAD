package com.musa.cad;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

/** Adds a simple DXF LAYER record to MusaCAD's editable working copy. */
public final class DxfLayerEditor {
    public static void addLayer(File file,String rawName)throws IOException{
        if(file==null||!file.isFile())throw new IOException("DXF çalışma dosyası bulunamadı");
        String name=rawName==null?"":rawName.trim();
        if(name.isEmpty())throw new IOException("Katman adı boş");
        byte[] bytes=Files.readAllBytes(file.toPath());
        String text=new String(bytes,StandardCharsets.ISO_8859_1);
        String newline=text.contains("\r\n")?"\r\n":"\n";
        List<String> lines=new ArrayList<>(Arrays.asList(text.split("\r?\n",-1)));
        if((lines.size()&1)!=0)throw new IOException("DXF etiket yapısı bozuk");
        boolean tables=false,sectionPending=false,layerTable=false,tablePending=false,seenLayerRecord=false;
        int countValueIndex=-1,insertLine=-1;
        for(int i=0;i+1<lines.size();i+=2){
            String code=lines.get(i).trim(),value=lines.get(i+1).trim();
            if("0".equals(code)&&"SECTION".equalsIgnoreCase(value)){sectionPending=true;continue;}
            if(sectionPending&&"2".equals(code)){tables="TABLES".equalsIgnoreCase(value);sectionPending=false;continue;}
            if("0".equals(code)&&"ENDSEC".equalsIgnoreCase(value)){tables=false;layerTable=false;tablePending=false;continue;}
            if(!tables)continue;
            if("0".equals(code)&&"TABLE".equalsIgnoreCase(value)){tablePending=true;continue;}
            if(tablePending&&"2".equals(code)){layerTable="LAYER".equalsIgnoreCase(value);tablePending=false;continue;}
            if(!layerTable)continue;
            if("0".equals(code)&&"LAYER".equalsIgnoreCase(value)){seenLayerRecord=true;continue;}
            if(!seenLayerRecord&&"70".equals(code)&&countValueIndex<0)countValueIndex=i+1;
            if(seenLayerRecord&&"2".equals(code)&&value.equalsIgnoreCase(name))throw new IOException("Katman zaten var: "+name);
            if("0".equals(code)&&"ENDTAB".equalsIgnoreCase(value)){insertLine=i;break;}
        }
        if(insertLine<0)throw new IOException("DXF LAYER tablosu bulunamadı");
        if(countValueIndex>=0){try{int count=Integer.parseInt(lines.get(countValueIndex).trim());lines.set(countValueIndex,Integer.toString(count+1));}catch(Exception ignored){}}
        lines.addAll(insertLine,Arrays.asList("  0","LAYER","  2",name," 70","0"," 62","7","  6","CONTINUOUS"));
        String out=String.join(newline,lines);
        Path temp=Files.createTempFile(file.toPath().getParent(),"musacad_layer_",".dxf");
        Files.write(temp,out.getBytes(StandardCharsets.ISO_8859_1),StandardOpenOption.TRUNCATE_EXISTING);
        try{Files.move(temp,file.toPath(),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);}
        catch(AtomicMoveNotSupportedException e){Files.move(temp,file.toPath(),StandardCopyOption.REPLACE_EXISTING);}
    }
    private DxfLayerEditor(){}
}
