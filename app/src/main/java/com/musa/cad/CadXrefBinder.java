package com.musa.cad;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * Safe, explicit subset of XREF bind. Resolves a user-provided external DXF
 * consisting exclusively of basic layer-0 entities into an existing XREF BLOCK.
 *
 * Unsupported nested blocks, custom layers/styles, proxies and handle-linked
 * objects are rejected, not silently discarded. Original files are untouched.
 */
public final class CadXrefBinder {
    private static final long MAX_BYTES=64L*1024L*1024L;
    private static final int MAX_PAIRS=1_000_000;
    private static final Set<String> SAFE_TYPES=new HashSet<>();
    static{
        for(String type:new String[]{"LINE","CIRCLE","ARC","LWPOLYLINE","POINT",
                "TEXT","SOLID","TRACE","3DFACE"})
            SAFE_TYPES.add(type);
    }
    private CadXrefBinder(){}

    private static final class Pair {
        final int code;final String value;
        Pair(int code,String value){this.code=code;this.value=value;}
    }
    private static List<Pair> read(File file)throws IOException{
        if(file==null||!file.isFile()||file.length()>MAX_BYTES)
            throw new IOException("XREF dosyası bulunamadı veya güvenlik boyut sınırını aşıyor");
        ArrayList<Pair> pairs=new ArrayList<>();
        try(BufferedReader in=Files.newBufferedReader(file.toPath(),StandardCharsets.UTF_8)){
            String code;
            while((code=in.readLine())!=null){
                FileTransfer.checkCancelled();
                String value=in.readLine();
                if(value==null)throw new IOException("DXF etiket çiftinde eksik değer");
                if(pairs.size()>=MAX_PAIRS)throw new IOException("XREF DXF etiket sayısı sınırı");
                try{pairs.add(new Pair(Integer.parseInt(code.trim()),value));}
                catch(NumberFormatException bad){throw new IOException("Geçersiz XREF DXF etiket kodu",bad);}
            }
        }
        return pairs;
    }
    private static String section(List<Pair> pairs,int start){
        return start+1<pairs.size()&&pairs.get(start+1).code==2
            ?pairs.get(start+1).value.trim().toUpperCase(Locale.ROOT):"";
    }
    private static int nextRecord(List<Pair> pairs,int index){
        int i=index+1;
        while(i<pairs.size()&&pairs.get(i).code!=0)i++;
        return i;
    }
    private static String tag(List<Pair> pairs,int from,int until,int code,String def){
        for(int i=from;i<until;i++)if(pairs.get(i).code==code)return pairs.get(i).value.trim();
        return def;
    }
    private static int flag(String value)throws IOException{
        try{return Integer.parseInt(value);}catch(NumberFormatException bad){
            throw new IOException("XREF BLOCK 70 bayrak alanı geçersiz",bad);
        }
    }

    private static List<Pair> allowedChildEntities(List<Pair> source)throws IOException{
        ArrayList<Pair> entities=new ArrayList<>();
        String section="";
        boolean foundSection=false;
        for(int i=0;i<source.size();){
            Pair p=source.get(i);
            if(p.code!=0){i++;continue;}
            int next=nextRecord(source,i);
            if("SECTION".equals(p.value)){section=section(source,i);i=next;continue;}
            if("ENDSEC".equals(p.value)){section="";i=next;continue;}
            if(!"ENTITIES".equals(section)){i=next;continue;}
            if("EOF".equals(p.value)){break;}
            foundSection=true;
            String type=p.value.trim().toUpperCase(Locale.ROOT);
            if(!SAFE_TYPES.contains(type))
                throw new IOException("XREF içinde henüz güvenle bağlanamayan nesne: "+type);
            for(int j=i;j<next;j++){
                Pair pair=source.get(j);
                if(pair.code==8&&!pair.value.trim().equalsIgnoreCase("0"))
                    throw new IOException("XREF özel katman içeriyor; kaynak katman tablosu korunamıyor");
                if(pair.code==6&&!pair.value.trim().equalsIgnoreCase("BYLAYER")
                    &&!pair.value.trim().equalsIgnoreCase("CONTINUOUS"))
                    throw new IOException("XREF özel çizgi tipi içeriyor");
                if(pair.code==7&&!pair.value.trim().equalsIgnoreCase("STANDARD"))
                    throw new IOException("XREF özel yazı tipi içeriyor");
                if(pair.code>=320&&pair.code<=369&&pair.code!=330)
                    throw new IOException("XREF handle-bağlantılı nesne içeriyor");
                if(pair.code==5||pair.code==330)continue;
                entities.add(pair);
            }
            i=next;
        }
        if(!foundSection||entities.isEmpty())
            throw new IOException("XREF içinde desteklenen ENTITIES geometrisi bulunamadı");
        return entities;
    }
    public static void bind(File parentDxf,File externalDxf,String xrefBlockName,File output)throws IOException{
        if(output==null||parentDxf==null||externalDxf==null
            ||xrefBlockName==null||xrefBlockName.trim().isEmpty())
            throw new IOException("XREF bağlama bilgileri eksik");
        String outputPath=output.getCanonicalPath();
        if(outputPath.equals(parentDxf.getCanonicalPath())||outputPath.equals(externalDxf.getCanonicalPath()))
            throw new IOException("XREF kaynağının üzerine yazmak yasaktır");
        List<Pair> parent=read(parentDxf);
        List<Pair> attached=allowedChildEntities(read(externalDxf));
        String target=xrefBlockName.trim().toUpperCase(Locale.ROOT);
        ArrayList<Pair> merged=new ArrayList<>(parent.size()+attached.size());
        String currentSection="";
        boolean inside=false,matched=false;
        for(int i=0;i<parent.size();){
            Pair entry=parent.get(i);
            if(entry.code!=0){
                if(!inside)merged.add(entry);
                i++;continue;
            }
            int next=nextRecord(parent,i);
            String kind=entry.value.trim().toUpperCase(Locale.ROOT);
            if("SECTION".equals(kind))currentSection=section(parent,i);
            if("ENDSEC".equals(kind))currentSection="";
            if("BLOCKS".equals(currentSection)&&"BLOCK".equals(kind)){
                String name=tag(parent,i+1,next,2,"").toUpperCase(Locale.ROOT);
                if(name.equals(target)){
                    if(matched)throw new IOException("Birden fazla aynı adlı XREF blok tanımı var");
                    int flags=flag(tag(parent,i+1,next,70,"0"));
                    if((flags&12)==0)throw new IOException("Hedef tanım bir XREF değil");
                    matched=true;inside=true;
                    merged.add(entry);
                    for(int j=i+1;j<next;j++){
                        Pair pair=parent.get(j);
                        if(pair.code==1)continue;
                        merged.add(pair.code==70?new Pair(70,Integer.toString(flags&~12)):pair);
                    }
                    i=next;continue;
                }
            }
            if(inside){
                if("ENDBLK".equals(kind)){
                    merged.addAll(attached);
                    for(int j=i;j<next;j++)merged.add(parent.get(j));
                    inside=false;
                    i=next;continue;
                }
                // XREF blocks must have no embedded records before binding.
                throw new IOException("Harici XREF blok zaten içerik taşıyor; veri kaybı önlendi");
            }
            for(int j=i;j<next;j++)merged.add(parent.get(j));
            i=next;
        }
        if(inside||!matched)throw new IOException("İstenen XREF blok tanımı bulunamadı veya eksik");
        if(merged.size()>MAX_PAIRS)throw new IOException("Bağlanan DXF etiket sayısı sınırı");
        boolean success=false;
        try(BufferedWriter out=Files.newBufferedWriter(output.toPath(),StandardCharsets.UTF_8)){
            for(Pair p:merged){
                FileTransfer.checkCancelled();
                out.write(Integer.toString(p.code));out.newLine();
                out.write(p.value);out.newLine();
            }
            success=true;
        }finally{if(!success)output.delete();}
    }
}
