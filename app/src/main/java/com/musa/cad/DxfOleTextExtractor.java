package com.musa.cad;

import java.io.*;
import java.util.*;
import java.util.regex.*;

/** Safely extracts bounded text hints from embedded DXF OLE payloads. */
public final class DxfOleTextExtractor {
    public static final class Result {
        public final boolean structured;
        public final String mode,text;
        public final int sheetCount,cellCount;
        private Result(boolean structured,String mode,String text,int sheetCount,int cellCount){
            this.structured=structured;this.mode=mode==null?"":mode;this.text=text==null?"":text;
            this.sheetCount=Math.max(0,sheetCount);this.cellCount=Math.max(0,cellCount);
        }
        public static Result empty(){return new Result(false,"","",0,0);}
    }

    private static final int MAX_SCAN=2*1024*1024,MAX_TEXT=16000,MAX_SNIPPETS=120;
    private static final Pattern SHEET=Pattern.compile("(?m)^\\s*[—-]{1,2}\\s*Sayfa\\s+\\d+\\s*[—-]{1,2}\\s*$",Pattern.CASE_INSENSITIVE|Pattern.UNICODE_CASE);
    private static final Pattern CELL=Pattern.compile("(?m)^[A-Za-z]{1,3}[1-9][0-9]{0,5}\\t.+$");

    public static Result extract(byte[] payload,String objectType){
        if(payload==null||payload.length==0)return Result.empty();
        String type=objectType==null?"":objectType.trim().toUpperCase(Locale.ROOT);
        int zip=findZip(payload);
        if(zip>=0){
            String fileName=officeFileName(type);
            if(fileName!=null){
                try{
                    String text=OfficeTextExtractor.extract(
                        new ByteArrayInputStream(payload,zip,payload.length-zip),fileName,mime(type));
                    String bounded=bound(text);
                    int sheets=count(SHEET,bounded),cells=count(CELL,bounded);
                    return new Result(true,"OOXML",bounded,sheets,cells);
                }catch(Exception ignored){}
            }
        }
        String hints=printableHints(payload);
        return hints.isEmpty()?Result.empty():new Result(false,"OLE_STRINGS",hints,0,0);
    }

    private static String officeFileName(String type){
        if("EXCEL".equals(type))return "embedded.xlsx";
        if("WORD".equals(type))return "embedded.docx";
        if("POWERPOINT".equals(type))return "embedded.pptx";
        return null;
    }
    private static String mime(String type){
        if("EXCEL".equals(type))return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        if("WORD".equals(type))return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        if("POWERPOINT".equals(type))return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        return "application/octet-stream";
    }

    private static int findZip(byte[]data){
        int limit=Math.min(data.length,MAX_SCAN);
        for(int i=0;i+3<limit;i++){
            if(data[i]=='P'&&data[i+1]=='K'&&(data[i+2]==3||data[i+2]==5||data[i+2]==7)&&(data[i+3]==4||data[i+3]==6||data[i+3]==8))return i;
        }
        return -1;
    }

    private static String printableHints(byte[]data){
        int limit=Math.min(data.length,MAX_SCAN);
        LinkedHashSet<String>out=new LinkedHashSet<>();
        collectAscii(data,limit,out);
        if(out.size()<MAX_SNIPPETS)collectUtf16Le(data,limit,out);
        StringBuilder b=new StringBuilder();
        for(String s:out){
            if(b.length()>0)b.append('\n');
            if(b.length()+s.length()>MAX_TEXT)break;
            b.append(s);
            if(out.size()>=MAX_SNIPPETS&&b.length()>MAX_TEXT/2)break;
        }
        return b.toString();
    }

    private static void collectAscii(byte[]data,int limit,Set<String>out){
        StringBuilder b=new StringBuilder();
        for(int i=0;i<limit&&out.size()<MAX_SNIPPETS;i++){
            int v=data[i]&255;
            if(v>=32&&v<=126){if(b.length()<220)b.append((char)v);}
            else{accept(b,out);b.setLength(0);}
        }
        accept(b,out);
    }

    private static void collectUtf16Le(byte[]data,int limit,Set<String>out){
        StringBuilder b=new StringBuilder();
        for(int i=0;i+1<limit&&out.size()<MAX_SNIPPETS;i+=2){
            int lo=data[i]&255,hi=data[i+1]&255;
            if(hi==0&&lo>=32&&lo<=126){if(b.length()<220)b.append((char)lo);}
            else{accept(b,out);b.setLength(0);}
        }
        accept(b,out);
    }

    private static void accept(StringBuilder raw,Set<String>out){
        if(raw.length()<4)return;
        String s=raw.toString().replaceAll("\\s+"," ").trim();
        if(s.length()<4)return;
        int letters=0,digits=0;
        for(int i=0;i<s.length();i++){char c=s.charAt(i);if(Character.isLetter(c))letters++;else if(Character.isDigit(c))digits++;}
        if(letters<3||letters+digits<s.length()/3)return;
        String n=s.toLowerCase(Locale.ROOT);
        if(n.startsWith("http://")||n.startsWith("https://")||n.contains("schemas.openxmlformats.org"))return;
        out.add(s);
    }

    private static int count(Pattern p,String text){int n=0;Matcher m=p.matcher(text==null?"":text);while(m.find())n++;return n;}
    private static String bound(String text){
        if(text==null)return "";
        String t=text.trim();
        return t.length()<=MAX_TEXT?t:t.substring(0,MAX_TEXT)+"\n[OLE metni sınırda kesildi.]";
    }
    private DxfOleTextExtractor(){}
}
