package com.musa.cad;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * Source-preserving editing of ordinary DXF INSERT/ATTRIB values. Dynamic
 * parameters, visibility states and proprietary evaluation graphs are not
 * reconstructed; this edits only the visible attribute text of a saved copy.
 */
public final class CadBlockAttributes {
    private static final long MAX_BYTES=128L*1024L*1024L;
    private static final int MAX_PAIRS=3_000_000;
    public static final class Attribute {
        public final int id;
        public final String block,tag,value,handle;
        Attribute(int id,String block,String tag,String value,String handle){
            this.id=id;this.block=block;this.tag=tag;this.value=value;this.handle=handle;
        }
    }
    private static final class Pair {
        final int code;final String value;
        Pair(int code,String value){this.code=code;this.value=value;}
    }
    private static final class Document {
        final ArrayList<Pair> pairs;
        final ArrayList<Attribute> attributes;
        final ArrayList<Integer> valueIndexes;
        Document(ArrayList<Pair> pairs,ArrayList<Attribute> attrs,ArrayList<Integer> idx){
            this.pairs=pairs;this.attributes=attrs;this.valueIndexes=idx;
        }
    }
    private CadBlockAttributes(){}

    private static String tag(List<Pair> pairs,int from,int end,int code){
        for(int i=from;i<end;i++)if(pairs.get(i).code==code)return pairs.get(i).value;
        return "";
    }
    private static int indexTag(List<Pair> pairs,int from,int end,int code){
        for(int i=from;i<end;i++)if(pairs.get(i).code==code)return i;
        return -1;
    }
    private static Document scan(File source)throws IOException{
        if(source==null||!source.isFile()||source.length()>MAX_BYTES)
            throw new IOException("ATTRIB kaynak DXF bulunamadı veya çok büyük");
        ArrayList<Pair> all=new ArrayList<>();
        try(BufferedReader in=Files.newBufferedReader(source.toPath(),StandardCharsets.UTF_8)){
            String code;
            while((code=in.readLine())!=null){
                FileTransfer.checkCancelled();
                String value=in.readLine();
                if(value==null)throw new IOException("Eksik DXF etiket değeri");
                if(all.size()>=MAX_PAIRS)throw new IOException("ATTRIB DXF etiket sınırı");
                try{all.add(new Pair(Integer.parseInt(code.trim()),value));}
                catch(NumberFormatException e){throw new IOException("Geçersiz ATTRIB DXF etiket kodu",e);}
            }
        }
        ArrayList<Attribute> attrs=new ArrayList<>();
        ArrayList<Integer> offsets=new ArrayList<>();
        String block="";
        boolean inEntities=false;
        for(int i=0;i<all.size();){
            Pair root=all.get(i);
            if(root.code!=0){i++;continue;}
            int end=i+1;while(end<all.size()&&all.get(end).code!=0)end++;
            String type=root.value.trim().toUpperCase(Locale.ROOT);
            if("SECTION".equals(type)){
                inEntities="ENTITIES".equalsIgnoreCase(tag(all,i+1,end,2));
                block="";
            }else if("ENDSEC".equals(type)){
                inEntities=false;block="";
            }else if(inEntities){
                if("INSERT".equals(type)){block=tag(all,i+1,end,2).trim();}
                else if("SEQEND".equals(type))block="";
                else if("ATTRIB".equals(type)&&!block.isEmpty()){
                    int at=indexTag(all,i+1,end,1);
                    if(at>=0){
                        attrs.add(new Attribute(attrs.size(),block,
                            tag(all,i+1,end,2).trim(),
                            all.get(at).value,tag(all,i+1,end,5).trim()));
                        offsets.add(at);
                    }
                }
            }
            i=end;
        }
        return new Document(all,attrs,offsets);
    }
    public static List<Attribute> list(File input)throws IOException{
        return Collections.unmodifiableList(scan(input).attributes);
    }

    public static void replace(File original,File target,int attributeId,String value)throws IOException{
        if(original==null||target==null
            ||original.getCanonicalPath().equals(target.getCanonicalPath()))
            throw new IOException("ATTRIB kaynak dosyanın üzerine yazılamaz");
        if(value==null||value.length()>8192||value.indexOf('\n')>=0
            ||value.indexOf('\r')>=0||value.indexOf('\0')>=0)
            throw new IOException("ATTRIB değeri geçersiz veya çok uzun");
        Document doc=scan(original);
        if(attributeId<0||attributeId>=doc.attributes.size())
            throw new IOException("Düzenlenecek ATTRIB kaydı bulunamadı");
        int at=doc.valueIndexes.get(attributeId);
        doc.pairs.set(at,new Pair(1,value));
        boolean complete=false;
        try(BufferedWriter out=Files.newBufferedWriter(target.toPath(),StandardCharsets.UTF_8)){
            for(Pair p:doc.pairs){
                FileTransfer.checkCancelled();
                out.write(Integer.toString(p.code));out.newLine();
                out.write(p.value);out.newLine();
            }
            complete=true;
        }finally{if(!complete)target.delete();}
    }
}
