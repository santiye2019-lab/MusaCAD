package com.musa.cad;

import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

/** Bounded record reader. Offsets are byte offsets, including for UTF-8 files. */
public final class DxfStream implements Closeable {
    public static final long MAX_BYTES=512L*1024*1024;
    private static final int MAX_LINE=65536,MAX_TAGS=200000;
    private final RandomAccessFile file;
    private final Charset charset;
    private final byte[] buffer=new byte[65536];
    private int position,available;
    private long offset;
    private String pendingType;
    public long nextRecordOffset;
    public DxfStream(File source,Charset charset)throws IOException {
        if(source.length()>MAX_BYTES)throw new IOException("DXF dosyası 512 MB sınırını aşıyor");
        this.file=new RandomAccessFile(source,"r");this.charset=charset;
    }
    public void seek(long offset)throws IOException {
        file.seek(offset);this.offset=offset;position=available=0;pendingType=null;
    }
    private String line()throws IOException {
        ByteArrayOutputStream bytes=new ByteArrayOutputStream(32);
        while(true){
            if(position==available){
                if(Thread.currentThread().isInterrupted())throw new InterruptedIOException("Yükleme iptal edildi");
                available=file.read(buffer);position=0;
                if(available<0){available=0;return bytes.size()==0?null:new String(bytes.toByteArray(),charset);}
            }
            int b=buffer[position++]&255;offset++;
            if(b==10)break;
            if(bytes.size()>=MAX_LINE)throw new IOException("DXF satırı çok uzun");
            bytes.write(b);
        }
        byte[] value=bytes.toByteArray();int length=value.length;
        if(length>0&&value[length-1]==13)length--;
        return new String(value,0,length,charset);
    }
    public DxfBlocks.Record next()throws IOException {
        if(Thread.currentThread().isInterrupted())throw new InterruptedIOException("Yükleme iptal edildi");
        String type=pendingType;pendingType=null;
        ArrayList<String> tags=new ArrayList<>();
        while(true){
            long start=offset;
            String code=line();
            if(code==null)return type==null?null:new DxfBlocks.Record(type,tags,0,tags.size());
            String value=line();if(value==null)throw new IOException("Eksik DXF etiket çifti");
            int number;
            try{number=Integer.parseInt(code.trim());}catch(NumberFormatException e){throw new IOException("Geçersiz DXF etiketi",e);}
            if(number<0||number>1071)throw new IOException("Geçersiz DXF etiket kodu");
            if(number==0){
                if(type!=null){pendingType=value.trim();nextRecordOffset=start;return new DxfBlocks.Record(type,tags,0,tags.size());}
                type=value.trim();
            }else if(type!=null&&keep(type,number)){
                if(tags.size()>=MAX_TAGS)throw new IOException("DXF nesnesi çok büyük");
                tags.add(Integer.toString(number));tags.add(value);
            }
        }
    }
    // Retain only fields used by geometry parsing and block transforms.
    private static boolean keep(String type,int code){
        if(code==8)return true;
        switch(type){
            case "SECTION":return code==2;
            case "BLOCK":case "INSERT":return code==1||code==2||code==10||code==20||code==30||code==41||code==42||code==50||code==70||code==71||code==210||code==220||code==230;
            case "LINE":return code==10||code==20||code==11||code==21;
            case "CIRCLE":case "ARC":return code==10||code==20||code==40||code==50||code==51;
            case "LWPOLYLINE":return code==10||code==20||code==70;
            case "TEXT":case "MTEXT":return code==1||code==3||code==10||code==20||code==11||code==21||code==40||code==50;
            default:return false;
        }
    }
    public void close()throws IOException {file.close();}
}
