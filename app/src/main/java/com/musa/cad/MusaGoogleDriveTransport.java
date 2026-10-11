package com.musa.cad;

import org.json.JSONObject;
import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.function.BooleanSupplier;
import javax.net.ssl.HttpsURLConnection;

/**
 * Experimental Google Drive transport for an OAuth-authorized Android account.
 * Not wired into automatic project sync until Drive consent and retention are implemented.
 * Never logs, persists, or sends the OAuth token to the Gandalf server.
 */
final class MusaGoogleDriveTransport {
    static final String SCOPE="https://www.googleapis.com/auth/drive.file";
    private static final int TIMEOUT=20000;
    private static final int CHUNK=1024*1024; // multiple of Google's 256 KiB boundary

    interface Progress { void confirmed(int percent); }
    static final class Upload {
        final String id;
        final long size;
        Upload(String id,long size){this.id=id;this.size=size;}
    }

    private static HttpsURLConnection connection(String target,String method,String token)
            throws IOException {
        URL u=new URL(target);
        if(!"https".equals(u.getProtocol())||!"www.googleapis.com".equals(u.getHost())||u.getPort()!=-1||u.getUserInfo()!=null||u.getRef()!=null||
           !u.getPath().matches("/upload/drive/v3/files(?:/.*)?"))
            throw new IOException("Untrusted Google Drive upload endpoint");
        if(token==null||token.isEmpty())throw new IOException("Drive authorization required");
        HttpsURLConnection c=(HttpsURLConnection)u.openConnection();
        c.setInstanceFollowRedirects(false);
        c.setConnectTimeout(TIMEOUT);c.setReadTimeout(TIMEOUT);
        c.setRequestMethod(method);
        c.setRequestProperty("Authorization","Bearer "+token);
        return c;
    }
    /** Verify cached upload against private Drive metadata, never trusting local preferences. */
    static boolean verified(String token,String id,File local)throws Exception {
        if(id==null||!id.matches("[A-Za-z0-9_-]{10,200}")||local==null||!local.isFile())
            return false;
        URL url=new URL("https://www.googleapis.com/drive/v3/files/"+id+
            "?fields=id,size,md5Checksum,trashed");
        HttpsURLConnection c=(HttpsURLConnection)url.openConnection();
        try {
            c.setInstanceFollowRedirects(false);
            c.setConnectTimeout(TIMEOUT);c.setReadTimeout(TIMEOUT);
            c.setRequestProperty("Authorization","Bearer "+token);
            int status=c.getResponseCode();
            if(status==404||status==410)return false;
            if(status!=200)throw new IOException("Drive metadata HTTP "+status);
            ByteArrayOutputStream bytes=new ByteArrayOutputStream();
            try(InputStream in=c.getInputStream()){
                byte[] buf=new byte[2048];int n;
                while((n=in.read(buf))!=-1){
                    if(bytes.size()+n>16384)throw new IOException("Drive metadata too large");
                    bytes.write(buf,0,n);
                }
            }
            JSONObject info=new JSONObject(bytes.toString("UTF-8"));
            if(!id.equals(info.optString("id"))||info.optBoolean("trashed",false)||
                info.optLong("size",-1)!=local.length())return false;
            String remoteHash=info.optString("md5Checksum","");
            if(remoteHash.isEmpty())return false;
            java.security.MessageDigest md=java.security.MessageDigest.getInstance("MD5");
            long beforeSize=local.length(),beforeModified=local.lastModified();
            try(InputStream in=new FileInputStream(local)){
                byte[] buf=new byte[65536];int n;
                while((n=in.read(buf))!=-1)md.update(buf,0,n);
            }
            if(beforeSize!=local.length()||beforeModified!=local.lastModified())return false;
            StringBuilder hex=new StringBuilder();
            for(byte value:md.digest())hex.append(String.format(java.util.Locale.ROOT,"%02x",value&255));
            return remoteHash.equalsIgnoreCase(hex.toString());
        }finally{c.disconnect();}
    }
    static String begin(String token,String name,long length,String parentId)throws Exception {
        if(name==null||name.isEmpty()||name.length()>240||length<1)
            throw new IOException("Invalid DWG file");
        JSONObject metadata=new JSONObject().put("name",name)
            .put("mimeType","application/octet-stream");
        if(parentId!=null&&!parentId.isEmpty())
            metadata.put("parents",new org.json.JSONArray().put(parentId));
        byte[] body=metadata.toString().getBytes(StandardCharsets.UTF_8);
        HttpsURLConnection c=connection(
            "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&fields=id,size,md5Checksum",
            "POST",token);
        try{
            c.setRequestProperty("Content-Type","application/json; charset=UTF-8");
            c.setRequestProperty("X-Upload-Content-Type","application/octet-stream");
            c.setRequestProperty("X-Upload-Content-Length",Long.toString(length));
            c.setDoOutput(true);c.setFixedLengthStreamingMode(body.length);
            try(OutputStream out=c.getOutputStream()){out.write(body);}
            int status=c.getResponseCode();
            if(status!=200&&status!=201)throw new IOException("Drive session HTTP "+status);
            String location=c.getHeaderField("Location");
            if(location==null)throw new IOException("Drive session location missing");
            // Validate redirect-free session location before ever sending bearer credentials.
            URL u=new URL(location);
            if(!"https".equals(u.getProtocol())||!"www.googleapis.com".equals(u.getHost())||u.getPort()!=-1||u.getUserInfo()!=null||u.getRef()!=null||
                !u.getPath().matches("/upload/drive/v3/files(?:/.*)?"))
                throw new IOException("Untrusted Drive session");
            return location;
        }finally{c.disconnect();}
    }
    private static long serverOffset(String token,String session,long total)throws Exception {
        HttpsURLConnection c=connection(session,"PUT",token);
        try{
            c.setRequestProperty("Content-Range","bytes */"+total);
            int status=c.getResponseCode();
            if(status==200||status==201)return total;
            if(status!=308)throw new IOException("Drive resume HTTP "+status);
            String range=c.getHeaderField("Range");
            if(range==null)return 0;
            if(!range.matches("bytes=0-[0-9]+"))throw new IOException("Bad Drive offset");
            long next=Long.parseLong(range.substring(8))+1;
            if(next>total)throw new IOException("Drive offset out of range");
            return next;
        }finally{c.disconnect();}
    }
    static Upload send(String token,String session,File file,Progress progress)throws Exception {
        return send(token,session,file,progress,()->false);
    }
    static Upload send(String token,String session,File file,Progress progress,
                       BooleanSupplier cancelled)throws Exception {
        if(file==null||!file.isFile()||file.length()<1)
            throw new IOException("Invalid local DWG");
        long total=file.length();
        long modified=file.lastModified();
        if(cancelled.getAsBoolean())throw new IOException("Drive upload superseded");
        long offset=serverOffset(token,session,total);
        if(offset==total)throw new IOException("Completed session requires Drive file metadata");
        byte[] buffer=new byte[CHUNK];
        try(RandomAccessFile in=new RandomAccessFile(file,"r")){
            while(offset<total){
                if(cancelled.getAsBoolean())throw new IOException("Drive upload superseded");
                if(file.length()!=total||file.lastModified()!=modified)
                    throw new IOException("DWG changed during Drive upload");
                int length=(int)Math.min(CHUNK,total-offset);
                in.seek(offset);in.readFully(buffer,0,length);
                HttpsURLConnection c=connection(session,"PUT",token);
                try{
                    c.setRequestProperty("Content-Type","application/octet-stream");
                    c.setRequestProperty("Content-Range","bytes "+offset+"-"+(offset+length-1)+"/"+total);
                    c.setDoOutput(true);c.setFixedLengthStreamingMode(length);
                    try(OutputStream out=c.getOutputStream()){out.write(buffer,0,length);}
                    int code=c.getResponseCode();
                    if(code==200||code==201){
                        ByteArrayOutputStream data=new ByteArrayOutputStream();
                        try(InputStream stream=c.getInputStream()){
                            byte[] tmp=new byte[2048];int n;
                            while((n=stream.read(tmp))!=-1){
                                if(data.size()+n>32768)throw new IOException("Drive response too large");
                                data.write(tmp,0,n);
                            }
                        }
                        JSONObject fileInfo=new JSONObject(data.toString("UTF-8"));
                        String id=fileInfo.optString("id","");
                        if(id.isEmpty()||fileInfo.optLong("size",-1)!=total)
                            throw new IOException("Unverified Drive completion");
                        if(file.length()!=total||file.lastModified()!=modified)
                            throw new IOException("DWG changed during Drive upload");
                        if(progress!=null)progress.confirmed(100);
                        return new Upload(id,total);
                    }
                    if(code!=308)throw new IOException("Drive chunk HTTP "+code);
                    String range=c.getHeaderField("Range");
                    if(range==null||!range.matches("bytes=0-[0-9]+"))
                        throw new IOException("Unconfirmed Drive chunk");
                    long next=Long.parseLong(range.substring(8))+1;
                    if(next<=offset||next>offset+length)
                        throw new IOException("Invalid Drive acknowledgement");
                    offset=next;
                    if(progress!=null)progress.confirmed((int)Math.min(99,offset*100/total));
                }finally{c.disconnect();}
            }
        }
        throw new IOException("Drive did not verify completion");
    }
    private MusaGoogleDriveTransport(){}
}
