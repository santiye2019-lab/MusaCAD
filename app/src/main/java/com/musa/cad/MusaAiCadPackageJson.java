package com.musa.cad;

import java.util.*;

/**
 * Bounded multi-drawing CAD package for Gandalf Cloud AI.
 * Raw DWG/DXF bytes are never included. Each drawing is projected through
 * MusaAiCadJson with a smaller per-drawing item budget.
 */
public final class MusaAiCadPackageJson {
    public static final int MAX_DRAWINGS=4;
    public static final int MAX_ITEMS_PER_DRAWING=300;
    public static final int MAX_LOCAL_SUMMARY_CHARS=12000;

    public static String build(Collection<MusaAiProjectPackage.Drawing>source,String localAuditSummary){
        ArrayList<MusaAiProjectPackage.Drawing> drawings=new ArrayList<>();
        if(source!=null)for(MusaAiProjectPackage.Drawing d:source){
            if(d==null||d.index==null)continue;
            drawings.add(d);
            if(drawings.size()>=MAX_DRAWINGS)break;
        }
        if(drawings.isEmpty())return "{}";

        StringBuilder out=new StringBuilder(64_000);
        out.append('{');
        field(out,"schema","musacad-cad-package/v1");out.append(',');
        name(out,"drawingCount");out.append(drawings.size()).append(',');
        name(out,"drawings");out.append('[');
        for(int i=0;i<drawings.size();i++){
            if(i>0)out.append(',');
            MusaAiProjectPackage.Drawing d=drawings.get(i);
            out.append('{');
            field(out,"fileName",clean(d.name,160));out.append(',');
            field(out,"detectedDiscipline",d.discipline.name());out.append(',');
            field(out,"disciplineLabel",d.discipline.label);out.append(',');
            name(out,"cad");
            out.append(MusaAiCadJson.build(d.index,d.name,MAX_ITEMS_PER_DRAWING));
            if(d.boq!=null&&!d.boq.isEmpty()){
                out.append(',');
                name(out,"boq");out.append('{');
                field(out,"name",clean(d.boq.name,160));out.append(',');
                name(out,"rowCount");out.append(d.boq.rows.size());
                out.append('}');
            }
            out.append('}');
        }
        out.append(']').append(',');
        field(out,"localAuditSummary",clean(localAuditSummary,MAX_LOCAL_SUMMARY_CHARS));out.append(',');
        name(out,"cloudPolicy");out.append('{');
        bool(out,"rawDrawingIncluded",false);out.append(',');
        bool(out,"automaticEditsAllowed",false);out.append(',');
        bool(out,"editActionsRequireUserApproval",true);out.append(',');
        bool(out,"packageEditToolsAllowed",false);
        out.append('}');
        out.append('}');
        return out.toString();
    }

    private static void field(StringBuilder out,String name,String value){
        name(out,name);quote(out,value);
    }
    private static void name(StringBuilder out,String name){
        quote(out,name);out.append(':');
    }
    private static void bool(StringBuilder out,String name,boolean value){
        name(out,name);out.append(value?"true":"false");
    }
    private static String clean(String value,int max){
        if(value==null)return "";
        String s=value.replace('\u0000',' ').trim();
        if(s.length()>max)s=s.substring(0,max);
        return s;
    }
    private static void quote(StringBuilder out,String value){
        out.append('"');
        String v=value==null?"":value;
        for(int i=0;i<v.length();i++){
            char c=v.charAt(i);
            switch(c){
                case '"':out.append('\\').append('"');break;
                case '\\':out.append('\\').append('\\');break;
                case '\b':out.append('\\').append('b');break;
                case '\f':out.append('\\').append('f');break;
                case '\n':out.append('\\').append('n');break;
                case '\r':out.append('\\').append('r');break;
                case '\t':out.append('\\').append('t');break;
                default:
                    if(c<0x20)out.append(String.format(Locale.ROOT,"\\u%04x",(int)c));
                    else out.append(c);
            }
        }
        out.append('"');
    }
    private MusaAiCadPackageJson(){}
}
