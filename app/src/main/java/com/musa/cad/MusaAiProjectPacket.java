package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;

/**
 * Privacy-minimized, deterministic drawing packet for Gandalf/cloud analysis.
 *
 * Intentionally excludes absolute file paths, device identity, license data,
 * account identifiers and arbitrary application preferences.
 */
public final class MusaAiProjectPacket {
    public static final int MAX_ITEMS=1200;
    public static final int MAX_TEXT_CHARS=240;
    public static final int MAX_LAYERS=300;

    public final String json;
    public final int includedItems;
    public final int totalItems;
    public final boolean truncated;

    private MusaAiProjectPacket(String json,int includedItems,int totalItems,boolean truncated){
        this.json=json;this.includedItems=includedItems;this.totalItems=totalItems;this.truncated=truncated;
    }

    public static MusaAiProjectPacket build(MusaAiDrawingIndex index,String displayName){
        if(index==null)return new MusaAiProjectPacket("{}",0,0,false);
        List<MusaAiDrawingIndex.Item>items=index.items();
        int total=items.size();
        int included=Math.min(MAX_ITEMS,total);
        boolean truncated=included<total;

        StringBuilder b=new StringBuilder(32768);
        b.append('{');
        field(b,"schema","musacad.project.v1");comma(b);
        field(b,"file_name",safeName(displayName));comma(b);
        field(b,"layout",clip(index.layout,120));comma(b);
        field(b,"unit",clip(index.unitName,40));comma(b);
        numberField(b,"entity_count",index.entityCount);comma(b);
        numberField(b,"indexed_item_count",total);comma(b);
        numberField(b,"included_item_count",included);comma(b);
        boolField(b,"truncated",truncated);comma(b);
        numberField(b,"ole_object_count",index.oleObjectCount);comma(b);
        numberField(b,"text_entity_count",index.textEntityCount());comma(b);

        b.append("\"layers\":[");
        int layerCount=0;
        for(String layer:index.allLayers){
            if(layerCount>=MAX_LAYERS)break;
            if(layerCount++>0)b.append(',');
            quote(b,clip(layer,160));
        }
        b.append("],"); 

        b.append("\"visible_layers\":[");
        int visibleCount=0;
        for(String layer:index.visibleLayers){
            if(visibleCount>=MAX_LAYERS)break;
            if(visibleCount++>0)b.append(',');
            quote(b,clip(layer,160));
        }
        b.append("],");

        b.append("\"type_counts\":{");
        int typePos=0;
        for(Map.Entry<String,Integer>e:index.typeCounts().entrySet()){
            if(typePos++>0)b.append(',');
            quote(b,clip(e.getKey(),80));b.append(':').append(Math.max(0,e.getValue()));
        }
        b.append("},");

        field(b,"mechanical_summary",clip(MusaAiMechanical.reportSection(index),4000));comma(b);

        b.append("\"items\":[");
        for(int i=0;i<included;i++){
            if(i>0)b.append(',');
            appendItem(b,items.get(i));
        }
        b.append("]}");
        return new MusaAiProjectPacket(b.toString(),included,total,truncated);
    }

    private static void appendItem(StringBuilder b,MusaAiDrawingIndex.Item item){
        b.append('{');
        numberField(b,"source_id",item.sourceId);comma(b);
        field(b,"type",clip(item.type,60));comma(b);
        field(b,"layer",clip(item.layer,160));comma(b);
        field(b,"text",clip(item.text,MAX_TEXT_CHARS));
        if(item.hasLength()){comma(b);decimalField(b,"length",item.length);}
        if(item.hasArea()){comma(b);decimalField(b,"area",item.area);}
        if(item.hasCenter()){
            comma(b);decimalField(b,"center_x",item.centerX);
            comma(b);decimalField(b,"center_y",item.centerY);
        }
        if(item.closedKnown){
            comma(b);boolField(b,"closed",item.closed);
        }
        b.append('}');
    }

    private static String safeName(String raw){
        String s=raw==null?"":raw.trim();
        s=s.replace('\\','/');
        int slash=s.lastIndexOf('/');
        if(slash>=0)s=s.substring(slash+1);
        return clip(s,180);
    }

    private static String clip(String s,int max){
        if(s==null)return "";
        String v=s.trim();
        if(v.length()<=max)return v;
        return v.substring(0,Math.max(0,max-1))+"…";
    }

    private static void field(StringBuilder b,String k,String v){quote(b,k);b.append(':');quote(b,v);}
    private static void numberField(StringBuilder b,String k,int v){quote(b,k);b.append(':').append(v);}
    private static void boolField(StringBuilder b,String k,boolean v){quote(b,k);b.append(':').append(v?"true":"false");}
    private static void decimalField(StringBuilder b,String k,double v){
        quote(b,k);b.append(':');
        if(!Double.isFinite(v)){b.append("null");return;}
        DecimalFormatSymbols symbols=DecimalFormatSymbols.getInstance(Locale.ROOT);
        b.append(new DecimalFormat("0.########",symbols).format(v));
    }
    private static void comma(StringBuilder b){b.append(',');}

    private static void quote(StringBuilder b,String value){
        b.append('"');
        String s=value==null?"":value;
        for(int i=0;i<s.length();i++){
            char c=s.charAt(i);
            switch(c){
                case '"': b.append("\\\"");break;
                case '\\': b.append("\\\\");break;
                case '\b': b.append("\\b");break;
                case '\f': b.append("\\f");break;
                case '\n': b.append("\\n");break;
                case '\r': b.append("\\r");break;
                case '\t': b.append("\\t");break;
                default:
                    if(c<0x20)b.append(String.format(Locale.ROOT,"\\u%04x",(int)c));
                    else b.append(c);
            }
        }
        b.append('"');
    }

    private MusaAiProjectPacket(){}
}
