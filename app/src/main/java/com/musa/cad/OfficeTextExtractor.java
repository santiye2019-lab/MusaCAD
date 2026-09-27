package com.musa.cad;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.*;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;

/** Lightweight offline text reader for modern Office Open XML documents. */
public final class OfficeTextExtractor {
    private static final int MAX_TEXT=1_500_000;
    private static final int MAX_ENTRY=16*1024*1024;
    private static final int MAX_TOTAL=64*1024*1024;

    private OfficeTextExtractor(){}

    public static String extract(InputStream input,String name,String mime)throws IOException{
        if(input==null)throw new IOException("Belge açılamadı");
        CadDocumentSupport.Kind kind=CadDocumentSupport.kind(name,mime);
        switch(kind){
            case DOCX:return extractDocx(input);
            case XLSX:return extractXlsx(input);
            case PPTX:return extractPptx(input);
            case TEXT:
            case CSV:return readText(input);
            default:throw new IOException("Bu belge biçimi metin okuyucuda desteklenmiyor");
        }
    }

    private static String extractDocx(InputStream in)throws IOException{
        Map<String,byte[]> entries=readZip(in,name->"word/document.xml".equals(name));
        byte[] xml=entries.get("word/document.xml");if(xml==null)throw new IOException("Word belge içeriği bulunamadı");
        return trim(extractRuns(xml,"p"));
    }

    private static String extractPptx(InputStream in)throws IOException{
        Pattern slide=Pattern.compile("ppt/slides/slide(\\d+)\\.xml");
        Map<String,byte[]> entries=readZip(in,name->slide.matcher(name).matches());
        ArrayList<String> names=new ArrayList<>(entries.keySet());names.sort(Comparator.comparingInt(n->numberIn(n,slide)));
        StringBuilder out=new StringBuilder();
        for(String entry:names){
            if(out.length()>0)out.append("\n\n");
            out.append("— Slayt ").append(numberIn(entry,slide)).append(" —\n");
            appendLimited(out,extractRuns(entries.get(entry),"p"));
            if(out.length()>=MAX_TEXT)break;
        }
        return trim(out.toString());
    }

    private static String extractXlsx(InputStream in)throws IOException{
        Pattern sheet=Pattern.compile("xl/worksheets/sheet(\\d+)\\.xml");
        Map<String,byte[]> entries=readZip(in,name->"xl/sharedStrings.xml".equals(name)||sheet.matcher(name).matches());
        List<String> shared=entries.containsKey("xl/sharedStrings.xml")?sharedStrings(entries.get("xl/sharedStrings.xml")):Collections.emptyList();
        ArrayList<String> names=new ArrayList<>();for(String key:entries.keySet())if(sheet.matcher(key).matches())names.add(key);
        names.sort(Comparator.comparingInt(n->numberIn(n,sheet)));
        StringBuilder out=new StringBuilder();
        for(String entry:names){
            if(out.length()>0)out.append("\n\n");
            out.append("— Sayfa ").append(numberIn(entry,sheet)).append(" —\n");
            appendLimited(out,extractSheet(entries.get(entry),shared));
            if(out.length()>=MAX_TEXT)break;
        }
        return trim(out.toString());
    }

    private static String readText(InputStream in)throws IOException{
        ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] buf=new byte[32*1024];int n,total=0;
        while((n=in.read(buf))!=-1){int keep=Math.min(n,MAX_TEXT-total);if(keep>0)out.write(buf,0,keep);total+=keep;if(total>=MAX_TEXT)break;}
        byte[] bytes=out.toByteArray();
        if(bytes.length>=2&&bytes[0]==(byte)0xFF&&bytes[1]==(byte)0xFE)return trim(new String(bytes,2,bytes.length-2,java.nio.charset.StandardCharsets.UTF_16LE));
        if(bytes.length>=2&&bytes[0]==(byte)0xFE&&bytes[1]==(byte)0xFF)return trim(new String(bytes,2,bytes.length-2,java.nio.charset.StandardCharsets.UTF_16BE));
        int off=bytes.length>=3&&bytes[0]==(byte)0xEF&&bytes[1]==(byte)0xBB&&bytes[2]==(byte)0xBF?3:0;
        return trim(new String(bytes,off,bytes.length-off,StandardCharsets.UTF_8));
    }

    private interface EntryFilter{boolean keep(String name);}
    private static Map<String,byte[]> readZip(InputStream input,EntryFilter filter)throws IOException{
        LinkedHashMap<String,byte[]> out=new LinkedHashMap<>();int total=0;
        try(ZipInputStream zip=new ZipInputStream(new BufferedInputStream(input))){
            ZipEntry e;byte[] buf=new byte[32*1024];
            while((e=zip.getNextEntry())!=null){
                String name=e.getName();if(e.isDirectory()||!filter.keep(name)){zip.closeEntry();continue;}
                ByteArrayOutputStream bytes=new ByteArrayOutputStream();int n,count=0;
                while((n=zip.read(buf))!=-1){count+=n;total+=n;if(count>MAX_ENTRY||total>MAX_TOTAL)throw new IOException("Belge çok büyük");bytes.write(buf,0,n);}
                out.put(name,bytes.toByteArray());zip.closeEntry();
            }
        }catch(ZipException e){throw new IOException("Office belgesi okunamadı",e);}
        return out;
    }

    private static String extractRuns(byte[] xml,String paragraphTag)throws IOException{
        StringBuilder out=new StringBuilder();
        parse(xml,new DefaultHandler(){
            boolean text;
            @Override public void startElement(String uri,String local,String qName,Attributes a){
                String n=name(local,qName);if("t".equals(n))text=true;else if("tab".equals(n))appendLimited(out,"\t");else if("br".equals(n)||"cr".equals(n))appendLimited(out,"\n");
            }
            @Override public void characters(char[] ch,int start,int length){if(text)appendLimited(out,new String(ch,start,length));}
            @Override public void endElement(String uri,String local,String qName){
                String n=name(local,qName);if("t".equals(n))text=false;else if(paragraphTag.equals(n))appendLimited(out,"\n");
            }
        });
        return out.toString();
    }

    private static List<String> sharedStrings(byte[] xml)throws IOException{
        ArrayList<String> result=new ArrayList<>();
        parse(xml,new DefaultHandler(){
            StringBuilder current;boolean text;
            @Override public void startElement(String uri,String local,String qName,Attributes a){String n=name(local,qName);if("si".equals(n))current=new StringBuilder();else if("t".equals(n)&&current!=null)text=true;}
            @Override public void characters(char[] ch,int start,int length){if(text&&current!=null&&current.length()<MAX_TEXT)current.append(ch,start,Math.min(length,MAX_TEXT-current.length()));}
            @Override public void endElement(String uri,String local,String qName){String n=name(local,qName);if("t".equals(n))text=false;else if("si".equals(n)&&current!=null){result.add(current.toString());current=null;}}
        });
        return result;
    }

    private static String extractSheet(byte[] xml,List<String> shared)throws IOException{
        StringBuilder out=new StringBuilder();
        parse(xml,new DefaultHandler(){
            String ref="",type="",value="";StringBuilder capture;boolean inCell;
            @Override public void startElement(String uri,String local,String qName,Attributes a){
                String n=name(local,qName);
                if("c".equals(n)){inCell=true;ref=a.getValue("r");type=a.getValue("t");value="";}
                else if(inCell&&("v".equals(n)||"t".equals(n)))capture=new StringBuilder();
            }
            @Override public void characters(char[] ch,int start,int length){if(capture!=null&&capture.length()<20000)capture.append(ch,start,Math.min(length,20000-capture.length()));}
            @Override public void endElement(String uri,String local,String qName){
                String n=name(local,qName);
                if(("v".equals(n)||"t".equals(n))&&capture!=null){value=capture.toString();capture=null;}
                else if("c".equals(n)&&inCell){
                    String shown=value;
                    if("s".equals(type)){try{int idx=Integer.parseInt(value.trim());if(idx>=0&&idx<shared.size())shown=shared.get(idx);}catch(Exception ignored){}}
                    if(shown!=null&&!shown.isEmpty()){appendLimited(out,(ref==null||ref.isEmpty()?"•":ref)+"\t"+shown+"\n");}
                    inCell=false;ref="";type="";value="";
                }
            }
        });
        return out.toString();
    }

    private static void parse(byte[] xml,DefaultHandler handler)throws IOException{
        try{
            SAXParserFactory f=SAXParserFactory.newInstance();f.setNamespaceAware(true);
            try{f.setFeature("http://apache.org/xml/features/disallow-doctype-decl",true);}catch(Exception ignored){}
            try{f.setFeature("http://xml.org/sax/features/external-general-entities",false);}catch(Exception ignored){}
            try{f.setFeature("http://xml.org/sax/features/external-parameter-entities",false);}catch(Exception ignored){}
            f.newSAXParser().parse(new ByteArrayInputStream(xml),handler);
        }catch(Exception e){throw new IOException("Belge XML içeriği okunamadı",e);}
    }

    private static String name(String local,String qName){if(local!=null&&!local.isEmpty())return local;int colon=qName==null?-1:qName.indexOf(':');return colon>=0?qName.substring(colon+1):qName;}
    private static int numberIn(String value,Pattern p){Matcher m=p.matcher(value);if(!m.matches())return Integer.MAX_VALUE;try{return Integer.parseInt(m.group(1));}catch(Exception e){return Integer.MAX_VALUE;}}
    private static void appendLimited(StringBuilder out,String value){if(value==null||value.isEmpty()||out.length()>=MAX_TEXT)return;out.append(value,0,Math.min(value.length(),MAX_TEXT-out.length()));}
    private static String trim(String value){
        if(value==null)return "";String normalized=value.replace("\r\n","\n").replace('\r','\n').replaceAll("[ \\t]+\\n","\n").replaceAll("\n{4,}","\n\n\n").trim();
        return normalized.length()>=MAX_TEXT?normalized+"\n\n[Belge metni görüntüleme sınırında kesildi.]":normalized;
    }
}
