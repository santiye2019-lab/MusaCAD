package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.graphics.pdf.PdfDocument;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.zip.*;

public final class MusaAiReportExport {
    public static File pdf(Context context,String title,String text)throws IOException{
        return pdf(context,title,text,null);
    }

    public static File pdf(Context context,String title,String text,Bitmap evidence)throws IOException{
        File file=File.createTempFile("MusaCAD_AI_Rapor_", ".pdf", exportDir(context));
        PdfDocument document=new PdfDocument();
        Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setColor(Color.BLACK);paint.setTextSize(10f);
        Paint head=new Paint(Paint.ANTI_ALIAS_FLAG);head.setColor(Color.BLACK);head.setTextSize(14f);head.setTypeface(Typeface.DEFAULT_BOLD);
        Paint sub=new Paint(Paint.ANTI_ALIAS_FLAG);sub.setColor(Color.BLACK);sub.setTextSize(11f);sub.setTypeface(Typeface.DEFAULT_BOLD);
        try{
            ArrayList<String> lines=wrap(text,paint,510f);int at=0,pageNo=1;
            do{
                PdfDocument.Page page=document.startPage(new PdfDocument.PageInfo.Builder(595,842,pageNo).create());
                Canvas canvas=page.getCanvas();canvas.drawColor(Color.WHITE);float y=42f;
                canvas.drawText(clean(title,"MusaCAD AI Raporu"),42f,y,head);y+=26f;
                if(pageNo==1&&evidence!=null&&!evidence.isRecycled()){
                    canvas.drawText("Çizim kanıt görüntüsü",42f,y,sub);y+=13f;
                    float maxW=510f,maxH=265f,ratio=Math.min(maxW/evidence.getWidth(),maxH/evidence.getHeight());
                    ratio=Math.min(1f,ratio);float w=evidence.getWidth()*ratio,h=evidence.getHeight()*ratio;
                    RectF dest=new RectF(42f,y,42f+w,y+h);
                    Paint imagePaint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
                    canvas.drawBitmap(evidence,null,dest,imagePaint);
                    Paint frame=new Paint(Paint.ANTI_ALIAS_FLAG);frame.setStyle(Paint.Style.STROKE);frame.setStrokeWidth(1f);frame.setColor(Color.DKGRAY);canvas.drawRect(dest,frame);
                    y+=h+18f;
                }
                while(at<lines.size()&&y<805f){
                    String line=lines.get(at++);
                    if(line.isEmpty()){y+=8f;continue;}
                    canvas.drawText(line,42f,y,paint);y+=15f;
                }
                paint.setTextSize(8f);canvas.drawText("MusaCAD AI • Sayfa "+pageNo,42f,825f,paint);paint.setTextSize(10f);
                document.finishPage(page);pageNo++;
            }while(at<lines.size());
            try(OutputStream out=new FileOutputStream(file)){document.writeTo(out);}
        }finally{document.close();}
        return file;
    }

    public static File docx(Context context,String title,String text)throws IOException{
        return docx(context,title,text,null);
    }

    public static File docx(Context context,String title,String text,Bitmap evidence)throws IOException{
        File file=File.createTempFile("MusaCAD_AI_Rapor_", ".docx", exportDir(context));
        byte[] imageBytes=png(evidence);boolean hasImage=imageBytes!=null&&imageBytes.length>0;
        long[] extent=hasImage?imageExtent(evidence):new long[]{0,0};
        try(ZipOutputStream zip=new ZipOutputStream(new BufferedOutputStream(new FileOutputStream(file)))){
            String contentTypes="<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"+
                "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"+
                "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"+
                (hasImage?"<Default Extension=\"png\" ContentType=\"image/png\"/>":"")+
                "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/></Types>";
            entry(zip,"[Content_Types].xml",contentTypes);
            entry(zip,"_rels/.rels","<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/></Relationships>");
            if(hasImage){
                entry(zip,"word/_rels/document.xml.rels","<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rIdEvidence\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"media/evidence.png\"/></Relationships>");
                binaryEntry(zip,"word/media/evidence.png",imageBytes);
            }
            entry(zip,"word/document.xml",wordXml(title,text,hasImage,extent[0],extent[1]));
        }
        return file;
    }

    private static String wordXml(String title,String text,boolean evidence,long cx,long cy){
        StringBuilder x=new StringBuilder("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"+
            "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "+
            "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "+
            "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" "+
            "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "+
            "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"><w:body>");
        para(x,clean(title,"MusaCAD AI Raporu"),true);
        para(x,"MusaCAD AI",true);
        if(evidence){
            para(x,"Çizim kanıt görüntüsü",true);
            imageParagraph(x,cx,cy);
        }
        String v=text==null?"":text.replace("\r\n","\n").replace('\r','\n');
        for(String line:v.split("\n",-1)){
            if(line.trim().isEmpty()){x.append("<w:p/>");continue;}
            String t=line.trim();
            boolean bold=t.matches("^\\d+\\..*")||(t.length()>5&&t.length()<90&&t.equals(t.toUpperCase(new Locale("tr","TR"))));
            para(x,line,bold);
        }
        x.append("<w:sectPr><w:pgSz w:w=\"11906\" w:h=\"16838\"/><w:pgMar w:top=\"900\" w:right=\"900\" w:bottom=\"900\" w:left=\"900\"/></w:sectPr></w:body></w:document>");
        return x.toString();
    }

    private static void imageParagraph(StringBuilder x,long cx,long cy){
        x.append("<w:p><w:r><w:drawing><wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">")
         .append("<wp:extent cx=\"").append(cx).append("\" cy=\"").append(cy).append("\"/>")
         .append("<wp:docPr id=\"1\" name=\"MusaCAD AI Kanıt Görüntüsü\"/>")
         .append("<wp:cNvGraphicFramePr><a:graphicFrameLocks noChangeAspect=\"1\"/></wp:cNvGraphicFramePr>")
         .append("<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"><pic:pic>")
         .append("<pic:nvPicPr><pic:cNvPr id=\"0\" name=\"evidence.png\"/><pic:cNvPicPr/></pic:nvPicPr>")
         .append("<pic:blipFill><a:blip r:embed=\"rIdEvidence\"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>")
         .append("<pic:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"").append(cx).append("\" cy=\"").append(cy).append("\"/></a:xfrm>")
         .append("<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></pic:spPr>")
         .append("</pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r></w:p>");
    }

    private static long[] imageExtent(Bitmap b){
        final long maxW=5943600L,maxH=3291840L;
        double scale=Math.min(maxW/(double)Math.max(1,b.getWidth()),maxH/(double)Math.max(1,b.getHeight()));
        return new long[]{Math.max(1L,Math.round(b.getWidth()*scale)),Math.max(1L,Math.round(b.getHeight()*scale))};
    }

    private static byte[] png(Bitmap b)throws IOException{
        if(b==null||b.isRecycled())return null;
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        if(!b.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("Çizim kanıt görüntüsü PNG olarak hazırlanamadı");
        return out.toByteArray();
    }

    private static void para(StringBuilder x,String s,boolean bold){
        x.append("<w:p><w:r><w:rPr>");if(bold)x.append("<w:b/>");
        x.append("</w:rPr><w:t xml:space=\"preserve\">").append(xml(s)).append("</w:t></w:r></w:p>");
    }
    private static void entry(ZipOutputStream z,String name,String value)throws IOException{
        z.putNextEntry(new ZipEntry(name));z.write(value.getBytes(StandardCharsets.UTF_8));z.closeEntry();
    }
    private static void binaryEntry(ZipOutputStream z,String name,byte[] value)throws IOException{
        z.putNextEntry(new ZipEntry(name));z.write(value);z.closeEntry();
    }
    private static ArrayList<String> wrap(String text,Paint p,float width){
        ArrayList<String> out=new ArrayList<>();
        for(String raw:(text==null?"":text).replace("\r\n","\n").replace('\r','\n').split("\n",-1)){
            if(raw.isEmpty()){out.add("");continue;}
            String left=raw;
            while(p.measureText(left)>width){
                int cut=left.length();while(cut>1&&p.measureText(left.substring(0,cut))>width)cut--;
                int space=left.lastIndexOf(' ',cut);if(space>8)cut=space;
                out.add(left.substring(0,cut).trim());left=left.substring(cut).trim();
            }
            out.add(left);
        }
        return out;
    }
    private static File exportDir(Context c){File d=new File(c.getCacheDir(),"exports");d.mkdirs();return d;}
    private static String xml(String s){return clean(s,"").replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("\"","&quot;").replace("'","&apos;");}
    private static String clean(String s,String f){return s==null||s.trim().isEmpty()?f:s.trim();}
    private MusaAiReportExport(){}
}
