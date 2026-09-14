import com.musa.cad.FileTransfer;
import java.io.*;
import java.util.Arrays;

public class FileTransferTest {
    public static void main(String[] args)throws Exception{
        byte[] input=new byte[150000];Arrays.fill(input,(byte)73);
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        if(FileTransfer.copy(new ByteArrayInputStream(input),out,input.length,n->{})!=input.length||!Arrays.equals(input,out.toByteArray()))throw new AssertionError("Copy corruption");
        ByteArrayOutputStream unlimited=new ByteArrayOutputStream();
        if(FileTransfer.copy(new ByteArrayInputStream(input),unlimited,1,n->{})!=input.length||!Arrays.equals(input,unlimited.toByteArray()))throw new AssertionError("Artificial size limit still active");
        try{FileTransfer.copy(new ByteArrayInputStream(new byte[0]),out,10,n->{});throw new AssertionError("Empty");}catch(IOException expected){}
        try{FileTransfer.copy(null,out,10,n->{});throw new AssertionError("Null");}catch(IOException expected){}
        ByteArrayOutputStream cancelled=new ByteArrayOutputStream();
        InputStream interrupted=new ByteArrayInputStream(input){public synchronized int read(byte[] b,int o,int l){int n=super.read(b,o,l);Thread.currentThread().interrupt();return n;}};
        try{FileTransfer.copy(interrupted,cancelled,input.length,n->{});throw new AssertionError("Cancellation");}catch(InterruptedIOException expected){}finally{Thread.interrupted();}
        if(cancelled.size()!=0)throw new AssertionError("Wrote data after cancellation");
        System.out.println("5 file transfer cases passed");
    }
}
