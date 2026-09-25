package com.musa.cad;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;

/** Atomic on-disk recovery snapshots for edited DXF work. Pure Java for regression testing. */
public final class RecoveryStore {
    public interface DataWriter {
        void write(OutputStream out) throws Exception;
    }

    public static final class Record {
        public final String id;
        public final String displayName;
        public final String sourceUri;
        public final long fingerprint;
        public final long savedAtMs;
        public final File dxfFile;

        Record(String id,String displayName,String sourceUri,long fingerprint,long savedAtMs,File dxfFile){
            this.id=id;
            this.displayName=displayName==null||displayName.trim().isEmpty()?"Kurtarılan çizim.dxf":displayName.trim();
            this.sourceUri=sourceUri==null?"":sourceUri;
            this.fingerprint=fingerprint;
            this.savedAtMs=savedAtMs;
            this.dxfFile=dxfFile;
        }
    }

    public static String idFor(String seed){
        String value=seed==null?"":seed.trim();
        if(value.isEmpty())value=UUID.randomUUID().toString();
        try{
            MessageDigest md=MessageDigest.getInstance("SHA-256");
            byte[] digest=md.digest(value.getBytes(StandardCharsets.UTF_8));
            StringBuilder out=new StringBuilder(24);
            for(int i=0;i<12;i++)out.append(String.format(Locale.ROOT,"%02x",digest[i]&0xff));
            return out.toString();
        }catch(Exception impossible){
            return Integer.toHexString(value.hashCode())+Long.toHexString(System.nanoTime());
        }
    }

    public static void write(File dir,String id,String displayName,String sourceUri,long fingerprint,long savedAtMs,DataWriter writer) throws Exception{
        if(dir==null||id==null||id.trim().isEmpty()||writer==null)throw new IllegalArgumentException("Recovery arguments missing");
        ensureDir(dir);
        String safe=safeId(id);
        File dataTmp=new File(dir,safe+".dxf.new");
        File dataFile=new File(dir,safe+".dxf");
        File dataBak=new File(dir,safe+".dxf.bak");
        File metaTmp=new File(dir,safe+".meta.new");
        File metaFile=new File(dir,safe+".meta");
        File metaBak=new File(dir,safe+".meta.bak");

        deleteQuietly(dataTmp);
        deleteQuietly(metaTmp);

        try(OutputStream raw=new FileOutputStream(dataTmp);BufferedOutputStream out=new BufferedOutputStream(raw,64*1024)){
            writer.write(out);
            out.flush();
        }
        if(dataTmp.length()==0L)throw new IOException("Recovery DXF is empty");

        Properties p=new Properties();
        p.setProperty("id",safe);
        p.setProperty("displayName",displayName==null?"":displayName);
        p.setProperty("sourceUri",sourceUri==null?"":sourceUri);
        p.setProperty("fingerprint",Long.toString(fingerprint));
        p.setProperty("savedAtMs",Long.toString(savedAtMs));
        try(OutputStream raw=new FileOutputStream(metaTmp);BufferedOutputStream out=new BufferedOutputStream(raw)){
            p.store(out,"MusaCAD recovery");
            out.flush();
        }

        replaceAtomicish(dataTmp,dataFile,dataBak);
        try{
            replaceAtomicish(metaTmp,metaFile,metaBak);
        }catch(Exception e){
            deleteQuietly(metaTmp);
            throw e;
        }
    }

    public static List<Record> list(File dir){
        ArrayList<Record> out=new ArrayList<>();
        if(dir==null||!dir.isDirectory())return out;
        File[] metas=dir.listFiles((d,name)->name.endsWith(".meta"));
        if(metas==null)return out;
        for(File meta:metas){
            Record r=read(meta);
            if(r!=null&&r.dxfFile.isFile()&&r.dxfFile.length()>0L)out.add(r);
        }
        out.sort((a,b)->Long.compare(b.savedAtMs,a.savedAtMs));
        return out;
    }

    public static Record findByFile(File dir,File file){
        if(dir==null||file==null)return null;
        String id=idFromRecoveryFile(dir,file);
        if(id==null)return null;
        File meta=new File(dir,id+".meta");
        return read(meta);
    }

    public static String idFromRecoveryFile(File dir,File file){
        if(dir==null||file==null)return null;
        try{
            File parent=file.getCanonicalFile().getParentFile();
            if(parent==null||!parent.equals(dir.getCanonicalFile()))return null;
        }catch(IOException e){return null;}
        String name=file.getName();
        if(!name.endsWith(".dxf"))return null;
        String id=name.substring(0,name.length()-4);
        return id.matches("[A-Za-z0-9_-]{4,80}")?id:null;
    }

    public static void delete(File dir,String id){
        if(dir==null||id==null||id.trim().isEmpty())return;
        String safe;
        try{safe=safeId(id);}catch(Exception e){return;}
        deleteQuietly(new File(dir,safe+".dxf"));
        deleteQuietly(new File(dir,safe+".dxf.new"));
        deleteQuietly(new File(dir,safe+".dxf.bak"));
        deleteQuietly(new File(dir,safe+".meta"));
        deleteQuietly(new File(dir,safe+".meta.new"));
        deleteQuietly(new File(dir,safe+".meta.bak"));
    }

    public static void prune(File dir,int keep){
        List<Record> records=list(dir);
        int limit=Math.max(0,keep);
        for(int i=limit;i<records.size();i++)delete(dir,records.get(i).id);
    }

    private static Record read(File meta){
        if(meta==null||!meta.isFile())return null;
        Properties p=new Properties();
        try(InputStream in=new BufferedInputStream(new FileInputStream(meta))){
            p.load(in);
            String id=safeId(p.getProperty("id",""));
            long fingerprint=Long.parseLong(p.getProperty("fingerprint","0"));
            long savedAtMs=Long.parseLong(p.getProperty("savedAtMs","0"));
            File dir=meta.getParentFile();
            File dxf=new File(dir,id+".dxf");
            return new Record(id,p.getProperty("displayName",""),p.getProperty("sourceUri",""),fingerprint,savedAtMs,dxf);
        }catch(Exception e){
            return null;
        }
    }

    private static void replaceAtomicish(File tmp,File target,File backup) throws IOException{
        deleteQuietly(backup);
        boolean hadTarget=target.exists();
        if(hadTarget&&!target.renameTo(backup))throw new IOException("Existing recovery snapshot could not be rotated");
        if(!tmp.renameTo(target)){
            if(hadTarget&&backup.exists())backup.renameTo(target);
            throw new IOException("Recovery snapshot could not be committed");
        }
        deleteQuietly(backup);
    }

    private static void ensureDir(File dir) throws IOException{
        if(dir.isDirectory())return;
        if(dir.exists()||!dir.mkdirs())throw new IOException("Recovery directory could not be created");
    }

    private static String safeId(String id){
        String s=id==null?"":id.trim();
        if(!s.matches("[A-Za-z0-9_-]{4,80}"))throw new IllegalArgumentException("Invalid recovery id");
        return s;
    }

    private static void deleteQuietly(File file){
        if(file!=null&&file.exists())file.delete();
    }

    private RecoveryStore(){}
}
