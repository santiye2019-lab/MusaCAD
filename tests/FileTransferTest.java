import com.musa.cad.FileTransfer;
import java.io.*;
import java.util.Arrays;

public class FileTransferTest {
    private static InputStream repeating(long size){
        return new InputStream(){
            long left=size;
            public int read(){if(left<=0)return -1;left--;return 65;}
            public int read(byte[] b,int off,int len){
                if(left<=0)return -1;int n=(int)Math.min(left,len);Arrays.fill(b,off,off+n,(byte)65);left-=n;return n;
            }
        };
    }

    public static void main(String[] args)throws Exception{
        byte[] input=new byte[150000];Arrays.fill(input,(byte)73);
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        if(FileTransfer.copy(new ByteArrayInputStream(input),out,input.length,n->{})!=input.length||!Arrays.equals(input,out.toByteArray()))throw new AssertionError("Copy corruption");
        try{FileTransfer.copy(new ByteArrayInputStream(input),new ByteArrayOutputStream(),input.length-1,n->{});throw new AssertionError("Limit");}catch(IOException expected){}
        try{FileTransfer.copy(new ByteArrayInputStream(new byte[0]),out,10,n->{});throw new AssertionError("Empty");}catch(IOException expected){}
        try{FileTransfer.copy(null,out,10,n->{});throw new AssertionError("Null");}catch(IOException expected){}
        ByteArrayOutputStream cancelled=new ByteArrayOutputStream();
        InputStream interrupted=new ByteArrayInputStream(input){public synchronized int read(byte[] b,int o,int l){int n=super.read(b,o,l);Thread.currentThread().interrupt();return n;}};
        try{FileTransfer.copy(interrupted,cancelled,input.length,n->{});throw new AssertionError("Cancellation");}catch(InterruptedIOException expected){}finally{Thread.interrupted();}
        if(cancelled.size()!=0)throw new AssertionError("Wrote data after cancellation");

        long thirtyThreeMb=33L*1024*1024;
        long copied=FileTransfer.copy(repeating(thirtyThreeMb),OutputStream.nullOutputStream(),32L*1024*1024,n->{});
        if(copied!=thirtyThreeMb)throw new AssertionError("Legacy 32 MB call was not upgraded for large drawings");

        System.out.println("6 file transfer cases passed");
    }
}
