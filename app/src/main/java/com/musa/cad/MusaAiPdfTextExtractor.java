package com.musa.cad;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.zip.InflaterInputStream;

/**
 * Bounded best-effort text extractor for text-based PDFs.
 *
 * It reads common unfiltered/Flate content streams and Tj/TJ text operators.
 * It is intentionally not OCR and does not claim support for scanned/image-only
 * PDFs or every custom PDF font encoding.
 */
public final class MusaAiPdfTextExtractor {
    private static final int MAX_PDF_BYTES=64*1024*1024;
    private static final int MAX_STREAM_BYTES=8*1024*1024;
    private static final int MAX_TEXT=1_500_000;
    private static final int MAX_STREAMS=2000;

    public static String extract(InputStream input)throws IOException{
        if(input==null)throw new IOException("PDF açılamadı");
        byte[] pdf=readBounded(input,MAX_PDF_BYTES);
        if(pdf.length<5||pdf[0]!='%'||pdf[1]!='P'||pdf[2]!='D'||pdf[3]!='F')
            throw new IOException("Geçerli PDF başlığı bulunamadı");

        String latin=new String(pdf,StandardCharsets.ISO_8859_1);
        StringBuilder out=new StringBuilder();
        int at=0,streams=0;
        while(streams<MAX_STREAMS&&out.length()<MAX_TEXT){
            int stream=latin.indexOf("stream",at);
            if(stream<0)break;
            int start=stream+6;
            if(start<latin.length()&&latin.charAt(start)=='\r')start++;
            if(start<latin.length()&&latin.charAt(start)=='\n')start++;
            int end=latin.indexOf("endstream",start);
            if(end<0)break;

            int dictStart=latin.lastIndexOf("<<",stream);
            int dictEnd=latin.lastIndexOf(">>",stream);
            String dict=dictStart>=0&&dictEnd>=dictStart&&stream-dictStart<8192
                ?latin.substring(dictStart,Math.min(stream,dictEnd+2)):"";

            if(end-start<=MAX_STREAM_BYTES){
                byte[] raw=Arrays.copyOfRange(pdf,start,end);
                byte[] decoded=decodeStream(raw,dict);
                if(decoded!=null&&decoded.length>0)extractTextOperators(decoded,out);
            }
            streams++;
            at=end+9;
        }
        String text=normalize(out.toString());
        if(text.length()<8)
            throw new IOException("PDF metin tabanlı görünmüyor veya yazı kodlaması desteklenmiyor; taranmış PDF için otomatik karşılaştırma yapılmadı");
        return text;
    }

    private static byte[] decodeStream(byte[]raw,String dict)throws IOException{
        if(dict.contains("/Filter")&&!dict.contains("/FlateDecode"))return null;
        if(!dict.contains("/FlateDecode"))return raw;
        try(InflaterInputStream in=new InflaterInputStream(new ByteArrayInputStream(trimTrailingEol(raw)))){
            return readBounded(in,MAX_STREAM_BYTES);
        }catch(Exception e){
            return null;
        }
    }

    private static byte[] trimTrailingEol(byte[]raw){
        int end=raw.length;
        while(end>0&&(raw[end-1]=='\r'||raw[end-1]=='\n'||raw[end-1]==' '||raw[end-1]=='\t'))end--;
        return end==raw.length?raw:Arrays.copyOf(raw,end);
    }

    private static void extractTextOperators(byte[]bytes,StringBuilder out){
        String s=new String(bytes,StandardCharsets.ISO_8859_1);
        int at=0;
        while(out.length()<MAX_TEXT){
            int bt=s.indexOf("BT",at);if(bt<0)break;
            int et=s.indexOf("ET",bt+2);if(et<0)break;
            extractTextBlock(s.substring(bt+2,et),out);
            at=et+2;
        }
    }

    private static void extractTextBlock(String block,StringBuilder out){
        int i=0;
        while(i<block.length()&&out.length()<MAX_TEXT){
            char c=block.charAt(i);
            if(c=='('){
                Parse p=parseLiteral(block,i);
                if(p!=null){
                    append(out,p.text);
                    i=p.next;
                    continue;
                }
            }else if(c=='<'&&i+1<block.length()&&block.charAt(i+1)!='<'){
                int end=block.indexOf('>',i+1);
                if(end>i){
                    append(out,decodeHex(block.substring(i+1,end)));
                    i=end+1;
                    continue;
                }
            }else if(c=='\''||c=='"'){
                append(out,"\n");
            }
            i++;
        }
        append(out,"\n");
    }

    private static final class Parse{
        final String text;final int next;
        Parse(String text,int next){this.text=text;this.next=next;}
    }

    private static Parse parseLiteral(String s,int start){
        StringBuilder out=new StringBuilder();int depth=1,i=start+1;
        while(i<s.length()){
            char c=s.charAt(i++);
            if(c=='\\'){
                if(i>=s.length())break;
                char e=s.charAt(i++);
                switch(e){
                    case 'n':out.append('\n');break;
                    case 'r':out.append('\r');break;
                    case 't':out.append('\t');break;
                    case 'b':out.append('\b');break;
                    case 'f':out.append('\f');break;
                    case '(':
                    case ')':
                    case '\\':out.append(e);break;
                    case '\r':
                        if(i<s.length()&&s.charAt(i)=='\n')i++;
                        break;
                    case '\n':break;
                    default:
                        if(e>='0'&&e<='7'){
                            int value=e-'0',count=1;
                            while(count<3&&i<s.length()){
                                char d=s.charAt(i);if(d<'0'||d>'7')break;
                                value=value*8+(d-'0');i++;count++;
                            }
                            out.append((char)(value&0xFF));
                        }else out.append(e);
                }
                continue;
            }
            if(c=='('){depth++;out.append(c);continue;}
            if(c==')'){
                depth--;if(depth==0)return new Parse(decodePdfString(out.toString()),i);
                out.append(c);continue;
            }
            out.append(c);
        }
        return null;
    }

    private static String decodePdfString(String raw){
        byte[] b=raw.getBytes(StandardCharsets.ISO_8859_1);
        if(b.length>=2&&b[0]==(byte)0xFE&&b[1]==(byte)0xFF)
            return new String(b,2,b.length-2,StandardCharsets.UTF_16BE);
        return new String(b,StandardCharsets.ISO_8859_1);
    }

    private static String decodeHex(String hex){
        String h=hex.replaceAll("\\s+","");
        if((h.length()&1)==1)h=h+"0";
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        for(int i=0;i+1<h.length();i+=2){
            try{out.write(Integer.parseInt(h.substring(i,i+2),16));}catch(Exception ignored){return "";}
        }
        byte[] b=out.toByteArray();
        if(b.length>=2&&b[0]==(byte)0xFE&&b[1]==(byte)0xFF)
            return new String(b,2,b.length-2,StandardCharsets.UTF_16BE);
        return new String(b,StandardCharsets.ISO_8859_1);
    }

    private static byte[] readBounded(InputStream in,int max)throws IOException{
        ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] buf=new byte[32*1024];int n,total=0;
        while((n=in.read(buf))!=-1){
            total+=n;if(total>max)throw new IOException("PDF güvenli okuma sınırını aşıyor");
            out.write(buf,0,n);
        }
        return out.toByteArray();
    }

    private static void append(StringBuilder out,String value){
        if(value==null||value.isEmpty()||out.length()>=MAX_TEXT)return;
        int keep=Math.min(value.length(),MAX_TEXT-out.length());
        out.append(value,0,keep);
        if(keep>0&&out.length()<MAX_TEXT&&!Character.isWhitespace(out.charAt(out.length()-1)))out.append(' ');
    }

    private static String normalize(String raw){
        String s=raw.replace('\r','\n').replace('\u0000',' ');
        s=s.replaceAll("[ \\t]{2,}"," ").replaceAll(" *\\n *","\n").replaceAll("\\n{3,}","\n\n").trim();
        return s.length()>MAX_TEXT?s.substring(0,MAX_TEXT):s;
    }

    private MusaAiPdfTextExtractor(){}
}
