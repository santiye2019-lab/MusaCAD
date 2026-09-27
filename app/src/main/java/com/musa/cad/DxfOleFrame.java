package com.musa.cad;

import java.io.ByteArrayOutputStream;
import java.util.*;

/** Decodes DXF OLE2FRAME boundary and the embedded binary payload when present. */
public final class DxfOleFrame {
    public static final class Result {
        public final double x1,y1,x2,y2;
        public final byte[] payload;
        public final String objectType;
        Result(double x1,double y1,double x2,double y2,byte[] payload,String objectType){
            this.x1=x1;this.y1=y1;this.x2=x2;this.y2=y2;this.payload=payload==null?new byte[0]:payload;this.objectType=objectType==null?"OLE":objectType;
        }
        public boolean valid(){return Double.isFinite(x1)&&Double.isFinite(y1)&&Double.isFinite(x2)&&Double.isFinite(y2)&&Math.abs(x2-x1)+Math.abs(y2-y1)>1e-9;}
        public boolean hasPayload(){return payload.length>0;}
    }

    private static final int MAX_PAYLOAD=32*1024*1024;

    public static Result parse(List<String>a,int from,int to){
        double x1=Double.NaN,y1=Double.NaN,x2=Double.NaN,y2=Double.NaN;
        ByteArrayOutputStream binary=new ByteArrayOutputStream();
        StringBuilder text=new StringBuilder();
        for(int i=from;i+1<to;i+=2){
            int code=intOf(a.get(i));String raw=a.get(i+1);double value;
            switch(code){
                case 10:value=d(raw);x1=value;break;
                case 20:value=d(raw);y1=value;break;
                case 11:value=d(raw);x2=value;break;
                case 21:value=d(raw);y2=value;break;
                case 1:case 2:case 3:case 1000:
                    if(text.length()<16384)text.append(' ').append(raw);
                    break;
                case 310:
                    appendHex(binary,raw);
                    break;
                default:break;
            }
        }
        byte[] payload=binary.toByteArray();
        return new Result(x1,y1,x2,y2,payload,classify(text.toString(),payload));
    }

    /** Returns PNG/JPEG/BMP bytes when a raster presentation is embedded in the OLE payload. */
    public static byte[] rasterPreview(byte[] payload){
        if(payload==null||payload.length<8)return null;
        int p=find(payload,new byte[]{(byte)0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A},0);
        if(p>=0)return Arrays.copyOfRange(payload,p,payload.length);
        p=find(payload,new byte[]{(byte)0xFF,(byte)0xD8,(byte)0xFF},0);
        if(p>=0){int end=findJpegEnd(payload,p+3);return Arrays.copyOfRange(payload,p,end>p?end:payload.length);}
        p=find(payload,new byte[]{0x42,0x4D},0);
        if(p>=0&&p+14<payload.length)return Arrays.copyOfRange(payload,p,payload.length);
        byte[] dib=extractDib(payload);return dib;
    }

    private static byte[] extractDib(byte[] data){
        for(int p=0;p+40<=data.length;p++){
            int header=le32(data,p);if(header!=40&&header!=108&&header!=124)continue;
            int width=le32(data,p+4),height=le32(data,p+8);int planes=le16(data,p+12),bpp=le16(data,p+14),compression=le32(data,p+16);
            if(width<=0||width>20000||height==0||Math.abs((long)height)>20000||planes!=1||!(bpp==1||bpp==4||bpp==8||bpp==16||bpp==24||bpp==32)||!(compression==0||compression==3))continue;
            int colors=le32(data,p+32);int palette=(bpp<=8?(colors>0?colors:(1<<bpp))*4:0);int masks=(compression==3&&header==40)?12:0;
            long row=((long)width*bpp+31L)/32L*4L;long pixels=row*Math.abs((long)height);long body=header+palette+masks+pixels;
            if(body<=0||body>Integer.MAX_VALUE||p+body>data.length)continue;
            int fileSize=(int)body+14,offset=14+header+palette+masks;byte[] bmp=new byte[fileSize];
            bmp[0]='B';bmp[1]='M';putLe32(bmp,2,fileSize);putLe32(bmp,10,offset);System.arraycopy(data,p,bmp,14,(int)body);return bmp;
        }
        return null;
    }

    private static String classify(String text,byte[] payload){
        StringBuilder probe=new StringBuilder(text==null?"":text);
        if(payload!=null&&payload.length>0){
            int limit=Math.min(payload.length,2*1024*1024);
            for(int i=0;i<limit;i++){int b=payload[i]&255;if(b>=32&&b<127)probe.append((char)b);else probe.append(' ');}
            for(int i=0;i+1<limit;i+=2){int lo=payload[i]&255,hi=payload[i+1]&255;if(hi==0&&lo>=32&&lo<127)probe.append((char)lo);}
        }
        String p=probe.toString().toLowerCase(Locale.ROOT);
        if(p.contains("excel.sheet")||p.contains("microsoft excel")||p.contains("worksheet"))return "EXCEL";
        if(p.contains("word.document")||p.contains("microsoft word"))return "WORD";
        if(p.contains("powerpoint")||p.contains("package.presentation"))return "POWERPOINT";
        if(p.contains("acroexch")||p.contains("adobe acrobat")||p.contains("pdf"))return "PDF";
        return "OLE";
    }

    private static void appendHex(ByteArrayOutputStream out,String raw){
        if(raw==null||out.size()>=MAX_PAYLOAD)return;String s=raw.trim();int hi=-1;
        for(int i=0;i<s.length()&&out.size()<MAX_PAYLOAD;i++){
            int v=Character.digit(s.charAt(i),16);if(v<0)continue;
            if(hi<0)hi=v;else{out.write((hi<<4)|v);hi=-1;}
        }
    }
    private static int find(byte[] data,byte[] needle,int start){
        outer:for(int i=Math.max(0,start);i+needle.length<=data.length;i++){for(int j=0;j<needle.length;j++)if(data[i+j]!=needle[j])continue outer;return i;}return-1;
    }
    private static int findJpegEnd(byte[]data,int start){for(int i=start;i+1<data.length;i++)if((data[i]&255)==0xFF&&(data[i+1]&255)==0xD9)return i+2;return-1;}
    private static int le16(byte[]d,int p){return(d[p]&255)|((d[p+1]&255)<<8);}
    private static int le32(byte[]d,int p){return(d[p]&255)|((d[p+1]&255)<<8)|((d[p+2]&255)<<16)|((d[p+3]&255)<<24);}
    private static void putLe32(byte[]d,int p,int v){d[p]=(byte)v;d[p+1]=(byte)(v>>>8);d[p+2]=(byte)(v>>>16);d[p+3]=(byte)(v>>>24);}
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double d(String s){try{return Double.parseDouble(s.trim());}catch(Exception e){return Double.NaN;}}
    private DxfOleFrame(){}
}
