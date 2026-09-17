package com.musa.cad;

import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

/**
 * Bounded streaming DXF record reader.
 * Keeps one code-0 record in memory at a time and tracks both byte and text-line offsets.
 */
public final class DxfStream implements Closeable {
    public static final long MAX_BYTES=512L*1024L*1024L;
    private static final int MAX_LINE_BYTES=1024*1024;
    private static final int MAX_RECORD_TAG_STRINGS=2_000_000;

    private final RandomAccessFile file;
    private final Charset charset;
    private final byte[] buffer=new byte[64*1024];
    private int position,available,lineIndex;
    private long offset;

    private String pendingType;
    private long pendingStartOffset;
    private int pendingStartLine;

    /** Byte/line position of the next code-0 record after the record just returned. */
    public long nextRecordOffset;
    public int nextRecordLine;

    public DxfStream(File source,Charset charset)throws IOException{
        if(source==null||!source.isFile())throw new IOException("DXF dosyası bulunamadı");
        if(source.length()>MAX_BYTES)throw new IOException("DXF dosyası 512 MB sınırını aşıyor");
        this.file=new RandomAccessFile(source,"r");
        this.charset=charset;
    }

    public void seek(long byteOffset,int sourceLine)throws IOException{
        if(byteOffset<0||byteOffset>file.length())throw new IOException("Geçersiz DXF konumu");
        file.seek(byteOffset);offset=byteOffset;lineIndex=Math.max(0,sourceLine);
        position=available=0;pendingType=null;pendingStartOffset=0;pendingStartLine=0;
        nextRecordOffset=byteOffset;nextRecordLine=lineIndex;
    }

    private String line()throws IOException{
        ByteArrayOutputStream bytes=new ByteArrayOutputStream(48);
        boolean any=false;
        while(true){
            if(position==available){
                FileTransfer.checkCancelled();
                available=file.read(buffer);position=0;
                if(available<0){
                    if(!any)return null;
                    lineIndex++;
                    return new String(bytes.toByteArray(),charset);
                }
            }
            int b=buffer[position++]&255;offset++;any=true;
            if(b==10){
                byte[] value=bytes.toByteArray();int length=value.length;
                if(length>0&&value[length-1]==13)length--;
                lineIndex++;
                return new String(value,0,length,charset);
            }
            if(bytes.size()>=MAX_LINE_BYTES)throw new IOException("DXF satırı çok uzun");
            bytes.write(b);
        }
    }

    public DxfBlocks.Record next()throws IOException{
        FileTransfer.checkCancelled();
        String type=pendingType;long recordStartOffset=pendingStartOffset;int recordStartLine=pendingStartLine;
        pendingType=null;
        ArrayList<String> tags=new ArrayList<>(32);

        while(true){
            long pairOffset=offset;int pairLine=lineIndex;
            String codeLine=line();
            if(codeLine==null){
                if(type==null)return null;
                nextRecordOffset=offset;nextRecordLine=lineIndex;
                return new DxfBlocks.Record(type,tags,0,tags.size(),recordStartLine+2,recordStartLine,lineIndex);
            }
            String value=line();
            if(value==null)throw new IOException("Eksik DXF etiket çifti");
            int code;
            try{code=Integer.parseInt(codeLine.trim());}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF etiketi",e);}
            if(code<0||code>1071)throw new IOException("Geçersiz DXF etiket kodu");

            if(code==0){
                String nextType=value.trim();
                if(type==null){
                    type=nextType;recordStartOffset=pairOffset;recordStartLine=pairLine;
                    continue;
                }
                pendingType=nextType;pendingStartOffset=pairOffset;pendingStartLine=pairLine;
                nextRecordOffset=pairOffset;nextRecordLine=pairLine;
                return new DxfBlocks.Record(type,tags,0,tags.size(),recordStartLine+2,recordStartLine,pairLine);
            }

            // Ignore preamble garbage before the first code-0 record, but never retain it.
            if(type==null)continue;
            if(tags.size()+2>MAX_RECORD_TAG_STRINGS)throw new IOException("DXF nesnesi çok büyük");
            tags.add(Integer.toString(code));tags.add(value);
        }
    }

    public void close()throws IOException{file.close();}
}
