package com.musa.cad;

import java.util.Locale;

/** File-type policy for MusaCAD's in-app document reader. */
public final class CadDocumentSupport {
    public enum Kind { PDF, DOCX, XLSX, PPTX, TEXT, CSV, LEGACY_WORD, LEGACY_EXCEL, LEGACY_POWERPOINT, UNKNOWN }

    private CadDocumentSupport(){}

    public static Kind kind(String name,String mime){
        String ext=extension(name),m=mime==null?"":mime.trim().toLowerCase(Locale.ROOT);
        if("pdf".equals(ext)||"application/pdf".equals(m))return Kind.PDF;
        if("docx".equals(ext)||"application/vnd.openxmlformats-officedocument.wordprocessingml.document".equals(m))return Kind.DOCX;
        if("xlsx".equals(ext)||"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet".equals(m))return Kind.XLSX;
        if("pptx".equals(ext)||"application/vnd.openxmlformats-officedocument.presentationml.presentation".equals(m))return Kind.PPTX;
        if("csv".equals(ext)||"text/csv".equals(m)||"application/csv".equals(m))return Kind.CSV;
        if("txt".equals(ext)||"text/plain".equals(m))return Kind.TEXT;
        if("doc".equals(ext)||"application/msword".equals(m))return Kind.LEGACY_WORD;
        if("xls".equals(ext)||"application/vnd.ms-excel".equals(m))return Kind.LEGACY_EXCEL;
        if("ppt".equals(ext)||"application/vnd.ms-powerpoint".equals(m))return Kind.LEGACY_POWERPOINT;
        return Kind.UNKNOWN;
    }

    public static boolean isDocument(String name,String mime){return kind(name,mime)!=Kind.UNKNOWN;}
    public static boolean readableInApp(String name,String mime){
        Kind k=kind(name,mime);return k==Kind.PDF||k==Kind.DOCX||k==Kind.XLSX||k==Kind.PPTX||k==Kind.TEXT||k==Kind.CSV;
    }
    public static boolean isLegacyOffice(String name,String mime){
        Kind k=kind(name,mime);return k==Kind.LEGACY_WORD||k==Kind.LEGACY_EXCEL||k==Kind.LEGACY_POWERPOINT;
    }
    public static String displayType(String name,String mime){
        switch(kind(name,mime)){
            case PDF:return "PDF";
            case DOCX:return "Word";
            case XLSX:return "Excel";
            case PPTX:return "PowerPoint";
            case TEXT:return "Metin";
            case CSV:return "CSV";
            case LEGACY_WORD:return "Eski Word (.doc)";
            case LEGACY_EXCEL:return "Eski Excel (.xls)";
            case LEGACY_POWERPOINT:return "Eski PowerPoint (.ppt)";
            default:return "Belge";
        }
    }
    public static String bestMime(String name,String mime){
        if(mime!=null&&!mime.trim().isEmpty()&&!"application/octet-stream".equalsIgnoreCase(mime))return mime;
        switch(kind(name,mime)){
            case PDF:return "application/pdf";
            case DOCX:return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
            case XLSX:return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
            case PPTX:return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
            case LEGACY_WORD:return "application/msword";
            case LEGACY_EXCEL:return "application/vnd.ms-excel";
            case LEGACY_POWERPOINT:return "application/vnd.ms-powerpoint";
            case CSV:return "text/csv";
            case TEXT:return "text/plain";
            default:return "application/octet-stream";
        }
    }
    private static String extension(String name){
        if(name==null)return "";String value=name.trim();int q=value.indexOf('?');if(q>=0)value=value.substring(0,q);int h=value.indexOf('#');if(h>=0)value=value.substring(0,h);
        int slash=Math.max(value.lastIndexOf('/'),value.lastIndexOf('\\')),dot=value.lastIndexOf('.');return dot>slash&&dot+1<value.length()?value.substring(dot+1).toLowerCase(Locale.ROOT):"";
    }
}
