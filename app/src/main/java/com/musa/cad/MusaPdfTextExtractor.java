package com.musa.cad;

import android.content.Context;
import com.tom_roush.pdfbox.android.PDFBoxResourceLoader;
import com.tom_roush.pdfbox.pdmodel.PDDocument;
import com.tom_roush.pdfbox.text.PDFTextStripper;
import java.io.*;

/** Offline bounded PDF text extractor for AI document analysis. */
public final class MusaPdfTextExtractor {
    private static final int MAX_PAGES=500;
    private static final int MAX_TEXT=1_500_000;

    public static final class Result {
        public final String text;
        public final int pageCount,processedPages;
        public final boolean truncated,likelyScanned;
        Result(String text,int pageCount,int processedPages,boolean truncated,boolean likelyScanned){
            this.text=text;this.pageCount=pageCount;this.processedPages=processedPages;
            this.truncated=truncated;this.likelyScanned=likelyScanned;
        }
    }

    private MusaPdfTextExtractor(){}

    public static Result extract(Context context,InputStream input)throws IOException{
        if(context==null)throw new IOException("PDF metin çıkarıcı başlatılamadı");
        if(input==null)throw new IOException("PDF dosyası açılamadı");
        PDFBoxResourceLoader.init(context.getApplicationContext());
        try(PDDocument document=PDDocument.load(new BufferedInputStream(input))){
            int pages=document.getNumberOfPages();
            if(pages<1)return new Result("",0,0,false,false);
            PDFTextStripper stripper=new PDFTextStripper();
            stripper.setSortByPosition(true);
            StringBuilder out=new StringBuilder();
            int limit=Math.min(pages,MAX_PAGES),processed=0;
            boolean truncated=pages>MAX_PAGES;
            for(int page=1;page<=limit&&out.length()<MAX_TEXT;page++){
                stripper.setStartPage(page);
                stripper.setEndPage(page);
                String value=stripper.getText(document);
                if(value!=null&&!value.trim().isEmpty()){
                    if(out.length()>0)out.append("\n\n");
                    out.append("— PDF Sayfa ").append(page).append(" —\n");
                    int remaining=MAX_TEXT-out.length();
                    if(value.length()>remaining){
                        out.append(value,0,Math.max(0,remaining));
                        truncated=true;
                    }else out.append(value);
                }
                processed=page;
            }
            String text=normalize(out.toString());
            boolean likelyScanned=pages>0&&text.replaceAll("\\s+","").length()<Math.min(120,pages*20);
            return new Result(text,pages,processed,truncated,likelyScanned);
        }catch(SecurityException e){
            throw new IOException("PDF parola/erişim koruması nedeniyle okunamadı",e);
        }catch(IOException e){
            throw e;
        }catch(Exception e){
            throw new IOException("PDF metni çıkarılamadı",e);
        }
    }

    private static String normalize(String value){
        if(value==null)return "";
        String s=value.replace("\r\n","\n").replace('\r','\n')
            .replaceAll("[ \\t]+\\n","\n").replaceAll("\n{4,}","\n\n\n").trim();
        if(s.length()>MAX_TEXT)s=s.substring(0,MAX_TEXT);
        return s;
    }
}
