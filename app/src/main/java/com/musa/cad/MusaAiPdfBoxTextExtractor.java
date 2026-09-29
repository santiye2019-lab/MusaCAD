package com.musa.cad;

import android.content.Context;
import com.tom_roush.pdfbox.android.PDFBoxResourceLoader;
import com.tom_roush.pdfbox.pdmodel.PDDocument;
import com.tom_roush.pdfbox.text.PDFTextStripper;
import java.io.*;
import java.nio.charset.StandardCharsets;

/**
 * Robust text-layer extractor for structural calculation PDFs.
 *
 * PDFBox handles ToUnicode/CMap/font encodings and common PDF stream filters.
 * If PDFBox cannot read a legacy/simple PDF, MusaCAD falls back to the bounded
 * local extractor. This class intentionally does not perform OCR.
 */
public final class MusaAiPdfBoxTextExtractor {
    private static final int MAX_PDF_BYTES=64*1024*1024;
    private static final int MAX_TEXT=1_500_000;
    private static final int MAX_PAGES=2500;

    public static String extract(Context context,InputStream input)throws IOException{
        if(context==null)throw new IOException("PDF okuyucu bağlamı bulunamadı");
        if(input==null)throw new IOException("PDF açılamadı");
        byte[] pdf=readBounded(input,MAX_PDF_BYTES);
        if(pdf.length<5||pdf[0]!='%'||pdf[1]!='P'||pdf[2]!='D'||pdf[3]!='F')
            throw new IOException("Geçerli PDF başlığı bulunamadı");

        IOException pdfBoxFailure=null;
        try{
            PDFBoxResourceLoader.init(context.getApplicationContext());
            try(PDDocument document=PDDocument.load(pdf)){
                int pages=document.getNumberOfPages();
                if(pages<1)throw new IOException("PDF içinde sayfa bulunamadı");
                PDFTextStripper stripper=new PDFTextStripper();
                stripper.setSortByPosition(true);
                stripper.setStartPage(1);
                stripper.setEndPage(Math.min(pages,MAX_PAGES));
                String text=normalize(stripper.getText(document));
                if(text.length()>=8){
                    if(pages>MAX_PAGES&&text.length()<MAX_TEXT)
                        text=limit(text+"\n\n[PDF "+pages+" sayfa; otomatik metin okuma ilk "+MAX_PAGES+" sayfa ile sınırlandı.]");
                    return text;
                }
                pdfBoxFailure=new IOException("PDF metin katmanı boş veya okunamayacak kadar kısa");
            }
        }catch(IOException e){
            pdfBoxFailure=e;
        }catch(Exception e){
            pdfBoxFailure=new IOException("PDFBox metin katmanını okuyamadı",e);
        }

        try{
            return MusaAiPdfTextExtractor.extract(new ByteArrayInputStream(pdf));
        }catch(IOException legacyFailure){
            String primary=pdfBoxFailure==null?"PDF metni okunamadı":safe(pdfBoxFailure.getMessage());
            String fallback=safe(legacyFailure.getMessage());
            throw new IOException(primary+"; yedek okuyucu: "+fallback,legacyFailure);
        }
    }

    private static byte[] readBounded(InputStream in,int max)throws IOException{
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] buf=new byte[32*1024];
        int n,total=0;
        while((n=in.read(buf))!=-1){
            total+=n;
            if(total>max)throw new IOException("PDF güvenli okuma sınırını aşıyor ("+(max/(1024*1024))+" MB)");
            out.write(buf,0,n);
        }
        return out.toByteArray();
    }

    private static String normalize(String raw){
        if(raw==null)return "";
        String s=raw.replace('\r','\n').replace('\u0000',' ');
        s=s.replaceAll("[ \\t]{2,}"," ")
            .replaceAll(" *\\n *","\n")
            .replaceAll("\\n{3,}","\n\n")
            .trim();
        return limit(s);
    }

    private static String limit(String value){
        if(value==null)return "";
        return value.length()<=MAX_TEXT?value:value.substring(0,MAX_TEXT)+"\n\n[PDF metni güvenli okuma sınırında kesildi.]";
    }

    private static String safe(String value){
        if(value==null||value.trim().isEmpty())return "bilinmeyen hata";
        String s=value.replace('\n',' ').replace('\r',' ').trim();
        return s.length()>240?s.substring(0,240)+"…":s;
    }

    private MusaAiPdfBoxTextExtractor(){}
}
