package com.musa.cad;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.zip.CRC32;

/**
 * Device-private 3D edit recovery. A bounded, checksummed sparse snapshot is
 * written when the editor stops (e.g. rotation or app backgrounding).
 * It NEVER modifies the original DWG/DXF or replaces a user export.
 *
 * A recovery snapshot can only be replayed against the exact original mesh
 * fingerprint (canonical source path + stat + XYZ + source slots).
 */
public final class Cad3dRecoveryJournal {
    private static final int MAGIC=0x4d334452, VERSION=1;
    private static final int MAX_CHANGED=50_000, MAX_BYTES=3_000_000;
    private Cad3dRecoveryJournal(){}

    public static final class Snapshot {
        private final int[] vertices;
        private final float[] current,checkpoint;
        private Snapshot(int[] vertices,float[] current,float[] checkpoint){
            this.vertices=vertices.clone();
            this.current=current.clone();
            this.checkpoint=checkpoint.clone();
        }
        public int changedVertices(){return vertices.length;}
        public int[] indices(){return vertices.clone();}
        /** Apply only after load() validated the matching source fingerprint. */
        public float[] savedGeometry(float[] original){
            float[] result=original.clone();
            for(int k=0;k<vertices.length;k++)
                System.arraycopy(checkpoint,k*3,result,vertices[k]*3,3);
            return result;
        }
        public void restoreCurrent(float[] xyz){
            for(int k=0;k<vertices.length;k++)
                System.arraycopy(current,k*3,xyz,vertices[k]*3,3);
        }
    }

    public static File pathFor(File directory,File source)throws IOException{
        if(directory==null||source==null)throw new IOException("Recovery location missing");
        String path=source.getCanonicalPath();
        byte[] digest=sha256(path.getBytes(StandardCharsets.UTF_8));
        return new File(directory,hex(digest)+".m3dr");
    }

    private static byte[] sha256(byte[] bytes)throws IOException{
        try{return MessageDigest.getInstance("SHA-256").digest(bytes);}
        catch(NoSuchAlgorithmException e){throw new IOException("SHA-256 unavailable",e);}
    }
    private static String hex(byte[] bytes){
        StringBuilder out=new StringBuilder(bytes.length*2);
        for(byte b:bytes)out.append(Character.forDigit((b>>4)&15,16))
            .append(Character.forDigit(b&15,16));
        return out.toString();
    }
    private static void feed(MessageDigest md,float[] values){
        ByteBuffer bytes=ByteBuffer.allocate(8192);
        for(float value:values){
            if(bytes.remaining()<4){md.update(bytes.array(),0,bytes.position());bytes.clear();}
            bytes.putInt(Float.floatToIntBits(value));
        }
        md.update(bytes.array(),0,bytes.position());
    }
    private static void feed(MessageDigest md,int[] values){
        ByteBuffer bytes=ByteBuffer.allocate(8192);
        for(int value:values){
            if(bytes.remaining()<4){md.update(bytes.array(),0,bytes.position());bytes.clear();}
            bytes.putInt(value);
        }
        md.update(bytes.array(),0,bytes.position());
    }
    /** Hashes the actual decoded XYZ/source-slot mapping without re-reading a huge DXF. */
    public static String fingerprint(File source,float[] original,int[] lines,int[] slots)
            throws IOException{
        if(source==null||!source.isFile()||original==null||lines==null||slots==null||
            original.length%3!=0||lines.length!=original.length/3||
            slots.length!=lines.length)throw new IOException("3D recovery fingerprint invalid");
        try{
            MessageDigest md=MessageDigest.getInstance("SHA-256");
            md.update(source.getCanonicalPath().getBytes(StandardCharsets.UTF_8));
            ByteBuffer stat=ByteBuffer.allocate(16);
            stat.putLong(source.length()).putLong(source.lastModified());
            md.update(stat.array());
            feed(md,original);feed(md,lines);feed(md,slots);
            return hex(md.digest());
        }catch(NoSuchAlgorithmException e){throw new IOException("SHA-256 unavailable",e);}
    }
    private static boolean valid(float value){
        return Float.isFinite(value)&&Math.abs(value)<=1e9f;
    }
    private static void validateIndex(int vertex,float[] original,int[] lines,int[] slots)
            throws IOException{
        if(vertex<0||vertex>=original.length/3||vertex>=lines.length||
            vertex>=slots.length||lines[vertex]<0||slots[vertex]<0)
            throw new IOException("3D recovery contains a noneditable vertex");
    }

    /**
     * Captures touched coordinates on the UI thread; no serialization, fsync or
     * file I/O is performed here. The returned geometry is immutable even if
     * the editor continues to move vertices before the writer runs.
     */
    public static Snapshot capture(float[] original,float[] current,int[] lines,
        int[] slots,Cad3dSaveCheckpoint checkpoint,Set<Integer> touched)
        throws IOException {
        if(original==null||current==null||original.length!=current.length||
            original.length%3!=0||lines==null||slots==null||
            lines.length!=original.length/3||slots.length!=lines.length||
            checkpoint==null||touched==null||touched.size()>MAX_CHANGED)
            throw new IOException("3D recovery snapshot input invalid");
        List<Integer> keys=new ArrayList<>(touched);
        Collections.sort(keys);
        int[] vertices=new int[keys.size()];
        float[] live=new float[keys.size()*3],saved=new float[keys.size()*3];
        for(int k=0;k<keys.size();k++){
            int vertex=keys.get(k);
            validateIndex(vertex,original,lines,slots);
            vertices[k]=vertex;
            float[] checkpointVertex=checkpoint.savedVertex(vertex);
            for(int axis=0;axis<3;axis++){
                float now=current[vertex*3+axis],before=checkpointVertex[axis];
                if(!valid(now)||!valid(before))
                    throw new IOException("3D recovery XYZ invalid");
                live[k*3+axis]=now;
                saved[k*3+axis]=before;
            }
        }
        return new Snapshot(vertices,live,saved);
    }

    /** Slow disk operation: call on a serialized background executor, never on UI. */
    public static void persist(File destination,String fingerprint,int totalVertices,
        Snapshot captured)throws IOException{
        if(destination==null||fingerprint==null||!fingerprint.matches("[0-9a-f]{64}")||
            totalVertices<0||captured==null||captured.vertices.length>MAX_CHANGED)
            throw new IOException("3D recovery write input invalid");
        ByteArrayOutputStream payload=new ByteArrayOutputStream(
            Math.min(MAX_BYTES,128+captured.vertices.length*28));
        try(DataOutputStream out=new DataOutputStream(payload)){
            out.writeInt(MAGIC);out.writeInt(VERSION);
            out.writeUTF(fingerprint);out.writeInt(totalVertices);
            out.writeInt(captured.vertices.length);
            int previous=-1;
            for(int k=0;k<captured.vertices.length;k++){
                int vertex=captured.vertices[k];
                if(vertex<=previous||vertex>=totalVertices)
                    throw new IOException("3D recovery snapshot indices invalid");
                previous=vertex;
                out.writeInt(vertex);
                for(int axis=0;axis<3;axis++)
                    out.writeFloat(captured.current[k*3+axis]);
                for(int axis=0;axis<3;axis++)
                    out.writeFloat(captured.checkpoint[k*3+axis]);
            }
        }
        byte[] data=payload.toByteArray();
        if(data.length+4>MAX_BYTES)throw new IOException("3D recovery exceeds size limit");
        CRC32 crc=new CRC32();crc.update(data);
        File parent=destination.getParentFile();
        if(parent==null||(!parent.isDirectory()&&!parent.mkdirs()))
            throw new IOException("3D recovery folder could not be created");
        File temp=File.createTempFile("musacad-3d-", ".tmp",parent);
        boolean committed=false;
        try{
            try(FileOutputStream stream=new FileOutputStream(temp);
                DataOutputStream out=new DataOutputStream(stream)){
                out.write(data);
                out.writeInt((int)crc.getValue());
                out.flush();
                stream.getFD().sync();
            }
            // Prior valid journal remains intact until replacement is complete.
            if(!temp.renameTo(destination))
                throw new IOException("3D recovery atomic replacement failed");
            committed=true;
        }finally{if(!committed)temp.delete();}
    }

    /** Original synchronous API retained for tests and non-UI callers. */
    public static void save(File destination,String fingerprint,float[] original,
        float[] current,int[] lines,int[] slots,Cad3dSaveCheckpoint checkpoint,
        Set<Integer> touched)throws IOException{
        Snapshot captured=capture(original,current,lines,slots,checkpoint,touched);
        persist(destination,fingerprint,original.length/3,captured);
    }

    public static Snapshot load(File file,String expectedFingerprint,float[] original,
        int[] lines,int[] slots)throws IOException{
        if(file==null||!file.isFile())return null;
        long size=file.length();
        if(size<4||size>MAX_BYTES||expectedFingerprint==null||
            original==null||lines==null||slots==null||
            original.length%3!=0||original.length/3!=lines.length||
            lines.length!=slots.length)
            throw new IOException("3D recovery is incomplete or too large");
        byte[] data=new byte[(int)size];
        try(FileInputStream in=new FileInputStream(file)){
            int pos=0,n;
            while(pos<data.length&&(n=in.read(data,pos,data.length-pos))>0)pos+=n;
            if(pos!=data.length||in.read()!=-1)
                throw new IOException("3D recovery is truncated");
        }
        CRC32 crc=new CRC32();crc.update(data,0,data.length-4);
        int savedCrc=ByteBuffer.wrap(data,data.length-4,4).getInt();
        if((int)crc.getValue()!=savedCrc)
            throw new IOException("3D recovery checksum mismatch");
        try(DataInputStream in=new DataInputStream(
                new ByteArrayInputStream(data,0,data.length-4))){
            if(in.readInt()!=MAGIC||in.readInt()!=VERSION)
                throw new IOException("Unknown 3D recovery format");
            if(!expectedFingerprint.equals(in.readUTF()))
                throw new IOException("Recovery belongs to a different source revision");
            if(in.readInt()!=original.length/3)
                throw new IOException("3D recovery vertex count changed");
            int count=in.readInt();
            if(count<0||count>MAX_CHANGED||count*28L>data.length)
                throw new IOException("3D recovery entry count invalid");
            int[] vertices=new int[count];
            float[] current=new float[count*3],saved=new float[count*3];
            int previous=-1;
            for(int k=0;k<count;k++){
                int vertex=in.readInt();
                if(vertex<=previous)throw new IOException("3D recovery indices unsorted");
                validateIndex(vertex,original,lines,slots);
                vertices[k]=vertex;previous=vertex;
                for(int axis=0;axis<3;axis++)current[k*3+axis]=in.readFloat();
                for(int axis=0;axis<3;axis++)saved[k*3+axis]=in.readFloat();
                for(int axis=0;axis<3;axis++){
                    if(!valid(current[k*3+axis])||!valid(saved[k*3+axis]))
                        throw new IOException("3D recovery XYZ invalid");
                }
            }
            if(in.available()!=0)throw new IOException("3D recovery has extra bytes");
            return new Snapshot(vertices,current,saved);
        }
    }
}
