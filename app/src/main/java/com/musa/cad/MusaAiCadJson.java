package com.musa.cad;

import java.util.*;

/**
 * Bounded, privacy-conscious CAD-JSON projection for cloud AI.
 *
 * Raw DWG/DXF bytes are never included here. Only the active drawing index,
 * visible metadata and lightweight measurements needed for engineering review
 * are serialized.
 */
public final class MusaAiCadJson {
    public static final int DEFAULT_MAX_ITEMS=1200;
    public static final int MAX_TEXT_CHARS=280;
    public static final int MAX_LAYERS=300;
    public static final int MAX_GEOMETRY_VERTICES=64;

    public static String build(MusaAiDrawingIndex index,String fileName){
        return build(index,fileName,DEFAULT_MAX_ITEMS);
    }

    public static String build(MusaAiDrawingIndex index,String fileName,int maxItems){
        if(index==null)return "{}";
        int limit=Math.max(1,Math.min(5000,maxItems));
        StringBuilder out=new StringBuilder(Math.min(1_000_000,4096+limit*180));
        out.append('{');
        field(out,"schema","musacad-cad-json/v1");out.append(',');
        field(out,"fileName",clean(fileName,160));out.append(',');
        field(out,"layout",clean(index.layout,120));out.append(',');
        field(out,"unit",clean(index.unitName,40));out.append(',');
        numberField(out,"entityCount",index.entityCount);out.append(',');
        numberField(out,"indexedItemCount",index.items().size());out.append(',');
        numberField(out,"oleObjectCount",index.oleObjectCount);out.append(',');

        out.append("\"layers\":");
        stringArray(out,index.allLayers,MAX_LAYERS);
        out.append(',');
        out.append("\"visibleLayers\":");
        stringArray(out,index.visibleLayers,MAX_LAYERS);
        out.append(',');

        out.append("\"typeCounts\":{");
        boolean first=true;
        for(Map.Entry<String,Integer>e:index.typeCounts().entrySet()){
            if(!first)out.append(',');first=false;
            quote(out,clean(e.getKey(),60));out.append(':').append(Math.max(0,e.getValue()));
        }
        out.append("},");

        out.append("\"items\":[");
        int written=0;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            if(written>=limit)break;
            if(written>0)out.append(',');
            appendItem(out,item);
            written++;
        }
        out.append("],");
        numberField(out,"itemsIncluded",written);out.append(',');
        out.append("\"truncated\":").append(index.items().size()>written?"true":"false").append(',');

        out.append("\"cloudPolicy\":{");
        out.append("\"rawDrawingIncluded\":false,");
        out.append("\"automaticEditsAllowed\":false,");
        out.append("\"editActionsRequireUserApproval\":true");
        out.append("}");
        out.append('}');
        return out.toString();
    }

    private static void appendItem(StringBuilder out,MusaAiDrawingIndex.Item item){
        out.append('{');
        numberField(out,"sourceId",item.sourceId);out.append(',');
        field(out,"type",clean(item.type,50));out.append(',');
        field(out,"layer",clean(item.layer,120));
        if(!item.text.isEmpty()){out.append(',');field(out,"text",clean(item.text,MAX_TEXT_CHARS));}
        if(item.hasLength()){out.append(',');decimalField(out,"length",item.length);}
        if(item.hasArea()){out.append(',');decimalField(out,"area",item.area);}
        if(item.hasCenter()){
            out.append(',');decimalField(out,"centerX",item.centerX);
            out.append(',');decimalField(out,"centerY",item.centerY);
        }
        if(item.closedKnown){out.append(",\"closed\":").append(item.closed?"true":"false");}
        appendEditableVertices(out,item);
        out.append('}');
    }

    private static void appendEditableVertices(StringBuilder out,MusaAiDrawingIndex.Item item){
        if(item==null||item.geometryKey==null||item.geometryKey.isEmpty())return;
        String type=item.type==null?"":item.type.toUpperCase(Locale.ROOT);
        if(!("LINE".equals(type)||"POLYLINE".equals(type)||"LWPOLYLINE".equals(type)))return;
        int first=item.geometryKey.indexOf('|'),last=item.geometryKey.indexOf("|c=");
        if(first<0||last<=first+1)return;
        String[] points=item.geometryKey.substring(first+1,last).split(";");
        if(points.length<2)return;
        out.append(",\"vertices\":[");
        int written=0;
        for(String point:points){
            if(written>=MAX_GEOMETRY_VERTICES)break;
            String[]xy=point.split(",");
            if(xy.length!=2)continue;
            try{
                double x=Long.parseLong(xy[0].trim())/100000d;
                double y=Long.parseLong(xy[1].trim())/100000d;
                if(written++>0)out.append(',');
                out.append('[').append(Double.toString(x)).append(',').append(Double.toString(y)).append(']');
            }catch(Exception ignored){}
        }
        out.append(']');
        if(points.length>written)out.append(",\"verticesTruncated\":true");
    }

    private static void field(StringBuilder out,String name,String value){
        quote(out,name);out.append(':');quote(out,value);
    }
    private static void numberField(StringBuilder out,String name,long value){
        quote(out,name);out.append(':').append(value);
    }
    private static void decimalField(StringBuilder out,String name,double value){
        quote(out,name);out.append(':');
        if(Double.isFinite(value))out.append(Double.toString(value));else out.append("null");
    }
    private static void stringArray(StringBuilder out,Collection<String>values,int max){
        out.append('[');int n=0;
        if(values!=null)for(String value:values){
            if(value==null||value.trim().isEmpty())continue;
            if(n>=max)break;
            if(n++>0)out.append(',');
            quote(out,clean(value,160));
        }
        out.append(']');
    }
    private static String clean(String value,int max){
        if(value==null)return "";
        String s=value.replace('\u0000',' ').trim();
        if(s.length()>max)s=s.substring(0,max);
        return s;
    }
    private static void quote(StringBuilder out,String value){
        out.append('"');
        for(int i=0;i<value.length();i++){
            char c=value.charAt(i);
            switch(c){
                case '"':out.append("\\\"");break;
                case '\\':out.append("\\\\");break;
                case '\b':out.append("\\b");break;
                case '\f':out.append("\\f");break;
                case '\n':out.append("\\n");break;
                case '\r':out.append("\\r");break;
                case '\t':out.append("\\t");break;
                default:
                    if(c<0x20)out.append(String.format(Locale.ROOT,"\\u%04x",(int)c));
                    else out.append(c);
            }
        }
        out.append('"');
    }

    private MusaAiCadJson(){}
}
