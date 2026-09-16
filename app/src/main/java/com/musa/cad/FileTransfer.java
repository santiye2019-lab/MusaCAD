package com.musa.cad;

import java.io.*;
import java.util.Locale;

public final class FileTransfer {
    public interface Progress { void copied(long bytes); }
    private static final long LEGACY_32_MB=32L*1024*1024;
    private static final long LARGE_DRAWING_LIMIT=256L*1024*1024;

    public static void checkCancelled()throws InterruptedIOException{
        if(Thread.currentThread().isInterrupted())throw new InterruptedIOException("Yükleme iptal edildi");
    }

    public static long copy(InputStream in,OutputStream out,long requestedLimit,Progress progress)throws IOException{
        if(in==null)throw new IOException("Dosyaya erişilemedi");
        long limit=requestedLimit<=LEGACY_32_MB?LARGE_DRAWING_LIMIT:requestedLimit;
        byte[] buffer=new byte[65536];long total=0,nextProgress=0;
        while(true){
            checkCancelled();int count=in.read(buffer);checkCancelled();
            if(count<0)break;if(count==0)continue;
            if(limit>0&&count>limit-total){
                long mb=Math.max(1,limit/(1024L*1024L));
                throw new IOException(String.format(Locale.getDefault(),"Dosya çok büyük; en fazla %d MB açılabilir",mb));
            }
            out.write(buffer,0,count);total+=count;
            if(total>=nextProgress){
                if(progress!=null)progress.copied(total);
                nextProgress=total+1048576;
            }
        }
        if(total==0)throw new IOException("Dosya boş");
        return total;
    }
    private FileTransfer(){}
}
