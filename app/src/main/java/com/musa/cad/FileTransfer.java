package com.musa.cad;

import java.io.*;

public final class FileTransfer {
    public interface Progress { void copied(long bytes); }
    public static void checkCancelled()throws InterruptedIOException{
        if(Thread.currentThread().isInterrupted())throw new InterruptedIOException("Yükleme iptal edildi");
    }
    public static long copy(InputStream in,OutputStream out,long limit,Progress progress)throws IOException{
        if(in==null)throw new IOException("Dosyaya erişilemedi");
        byte[] buffer=new byte[65536];long total=0,nextProgress=0;
        while(true){
            checkCancelled();int count=in.read(buffer);checkCancelled();
            if(count<0)break;if(count==0)continue;
            if(count>limit-total)throw new IOException("Dosya çok büyük; en fazla "+(limit/1048576)+" MB açılabilir");
            out.write(buffer,0,count);total+=count;
            if(total>=nextProgress){progress.copied(total);nextProgress=total+1048576;}
        }
        if(total==0)throw new IOException("Dosya boş");
        return total;
    }
    private FileTransfer(){}
}
