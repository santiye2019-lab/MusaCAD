package com.musa.cad;

import org.json.JSONArray;
import org.json.JSONObject;
import java.util.*;

/**
 * Whitelisted executor for Gandalf CAD proposals.
 *
 * Cloud tool calls are never executed directly. MainActivity must first show
 * the preview and obtain explicit user approval. This executor then applies
 * only known bounded actions and rolls the complete batch back if one approved
 * mutation cannot be completed.
 */
public final class MusaAiActionExecutor {
    private static final int MAX_ACTIONS=50;
    private static final int MAX_HIGHLIGHT_IDS=200;
    private static final int MAX_TEXT_CHARS=4000;
    private static final int MAX_LAYER_CHARS=128;
    private static final int MAX_POLYLINE_POINTS=128;
    private static final int MAX_BLOCK_LABEL_CHARS=120;

    public static final class Preview {
        public final int actionCount,mutationCount,deleteCount,highlightCount,unsupportedCount;
        public final String text;
        Preview(int actionCount,int mutationCount,int deleteCount,int highlightCount,int unsupportedCount,String text){
            this.actionCount=actionCount;this.mutationCount=mutationCount;this.deleteCount=deleteCount;
            this.highlightCount=highlightCount;this.unsupportedCount=unsupportedCount;this.text=text==null?"":text;
        }
        public boolean hasActions(){return actionCount>0;}
    }

    public static final class ApplyResult {
        public final boolean success,rolledBack;
        public final int mutationsApplied,highlighted,skipped;
        public final String message;
        ApplyResult(boolean success,boolean rolledBack,int mutationsApplied,int highlighted,int skipped,String message){
            this.success=success;this.rolledBack=rolledBack;this.mutationsApplied=mutationsApplied;
            this.highlighted=highlighted;this.skipped=skipped;this.message=message==null?"":message;
        }
    }

    public static Preview preview(List<MusaAiCloudService.Action> actions){
        if(actions==null||actions.isEmpty())return new Preview(0,0,0,0,0,"Bekleyen Gandalf çizim önerisi yok.");
        int count=Math.min(actions.size(),MAX_ACTIONS),mutations=0,deletes=0,highlights=0,unsupported=0;
        StringBuilder out=new StringBuilder();
        for(int i=0;i<count;i++){
            MusaAiCloudService.Action action=actions.get(i);
            String name=action==null?"":action.name;
            String label=label(name);
            if("cad_highlight_entities".equals(name))highlights++;
            else if(isMutation(name)){mutations++;if("cad_delete_entity".equals(name))deletes++;}
            else unsupported++;
            if(i<20){
                out.append(i==0?"":"\n").append("• ").append(label);
                if(action!=null&&!action.reason.trim().isEmpty())out.append(" — ").append(limit(action.reason.trim(),180));
            }
        }
        if(count>20)out.append("\n• … +").append(count-20).append(" öneri");
        if(actions.size()>MAX_ACTIONS)out.append("\n• Güvenlik sınırı nedeniyle ilk ").append(MAX_ACTIONS).append(" öneri değerlendirilecek.");
        return new Preview(count,mutations,deletes,highlights,unsupported,out.toString());
    }

    public static ApplyResult apply(CadView cad,List<MusaAiCloudService.Action> actions){
        if(cad==null)return new ApplyResult(false,false,0,0,0,"Çizim görünümü hazır değil.");
        if(actions==null||actions.isEmpty())return new ApplyResult(false,false,0,0,0,"Bekleyen Gandalf çizim önerisi yok.");
        if(actions.size()>MAX_ACTIONS)return new ApplyResult(false,false,0,0,0,"Tek seferde en fazla "+MAX_ACTIONS+" Gandalf işlemi uygulanabilir.");

        CadView.SessionState before=cad.captureSessionState();
        LinkedHashSet<Integer> highlights=new LinkedHashSet<>();
        int applied=0,skipped=0;
        try{
            for(MusaAiCloudService.Action action:actions){
                if(action==null||action.name==null){skipped++;continue;}
                JSONObject a=parse(action.arguments);
                switch(action.name){
                    case "cad_highlight_entities":{
                        JSONArray ids=a.optJSONArray("sourceIds");
                        if(ids==null){skipped++;break;}
                        for(int i=0;i<ids.length()&&highlights.size()<MAX_HIGHLIGHT_IDS;i++){
                            int id=ids.optInt(i,-1);if(id>=0)highlights.add(id);
                        }
                        break;
                    }
                    case "cad_move_entity":{
                        int id=requiredSourceId(a);
                        double dx=requiredFinite(a,"dx"),dy=requiredFinite(a,"dy");
                        if(!cad.applyAiMoveSource(id,dx,dy))throw new IllegalStateException("Taşıma uygulanamadı • sourceId "+id);
                        applied++;break;
                    }
                    case "cad_delete_entity":{
                        int id=requiredSourceId(a);
                        if(!cad.applyAiDeleteSource(id))throw new IllegalStateException("Silme uygulanamadı • sourceId "+id);
                        applied++;break;
                    }
                    case "cad_change_layer":{
                        int id=requiredSourceId(a);String layer=requiredLayer(a);
                        if(!cad.applyAiChangeLayer(id,layer))throw new IllegalStateException("Katman değişikliği uygulanamadı • sourceId "+id+" • "+layer);
                        applied++;break;
                    }
                    case "cad_add_line":{
                        String layer=requiredLayer(a);
                        double x1=requiredFinite(a,"x1"),y1=requiredFinite(a,"y1"),x2=requiredFinite(a,"x2"),y2=requiredFinite(a,"y2");
                        if(!cad.applyAiAddLine(x1,y1,x2,y2,layer))throw new IllegalStateException("Çizgi eklenemedi • katman "+layer);
                        applied++;break;
                    }
                    case "cad_add_text":{
                        String layer=requiredLayer(a);String value=requiredText(a,"text");
                        double x=requiredFinite(a,"x"),y=requiredFinite(a,"y");
                        if(!cad.applyAiAddText(x,y,value,layer))throw new IllegalStateException("Metin eklenemedi • katman "+layer);
                        applied++;break;
                    }
                    case "cad_replace_text":{
                        int id=requiredSourceId(a);String value=requiredText(a,"text");
                        if(!cad.applyAiReplaceTextSource(id,value))throw new IllegalStateException("Metin değiştirilemedi • sourceId "+id);
                        applied++;break;
                    }
                    case "cad_add_polyline":{
                        String layer=requiredLayer(a);boolean closed=a.optBoolean("closed",false);
                        double[] points=requiredPoints(a,"points",closed?3:2,MAX_POLYLINE_POINTS);
                        if(!cad.applyAiAddPolyline(points,closed,layer))throw new IllegalStateException("Polyline eklenemedi • katman "+layer);
                        applied++;break;
                    }
                    case "cad_offset_entity":{
                        int id=requiredSourceId(a);double distance=requiredFinite(a,"distance");
                        if(Math.abs(distance)<1e-9d)throw new IllegalArgumentException("Offset mesafesi sıfır olamaz");
                        if(!cad.applyAiOffsetSource(id,distance))throw new IllegalStateException("Offset uygulanamadı • sourceId "+id);
                        applied++;break;
                    }
                    case "cad_trim_line":{
                        int target=requiredInt(a,"targetSourceId"),boundary=requiredInt(a,"boundarySourceId");
                        double pickX=requiredFinite(a,"pickX"),pickY=requiredFinite(a,"pickY");
                        if(!cad.applyAiTrimLine(target,boundary,pickX,pickY))throw new IllegalStateException("TRIM uygulanamadı • hedef "+target+" • sınır "+boundary);
                        applied++;break;
                    }
                    case "cad_extend_line":{
                        int target=requiredInt(a,"targetSourceId"),boundary=requiredInt(a,"boundarySourceId");
                        if(!cad.applyAiExtendLine(target,boundary))throw new IllegalStateException("EXTEND uygulanamadı • hedef "+target+" • sınır "+boundary);
                        applied++;break;
                    }
                    case "cad_continue_path":{
                        int id=requiredSourceId(a);String from=a.optString("from","").trim().toLowerCase(Locale.ROOT);
                        if(!"start".equals(from)&&!"end".equals(from))throw new IllegalArgumentException("Devam yönü start veya end olmalı");
                        double[] points=requiredPoints(a,"points",1,MAX_POLYLINE_POINTS);
                        if(!cad.applyAiContinuePath(id,"start".equals(from),points))throw new IllegalStateException("Hat devam ettirilemedi • sourceId "+id);
                        applied++;break;
                    }
                    case "cad_add_pipe_note":{
                        String layer=requiredLayer(a);
                        double x=requiredFinite(a,"x"),y=requiredFinite(a,"y");
                        String diameter=limitedString(a,"diameter",80),slope=limitedString(a,"slope",80);
                        String value=pipeNote(diameter,slope);
                        if(value.isEmpty())throw new IllegalArgumentException("Çap/eğim notu boş olamaz");
                        if(!cad.applyAiAddText(x,y,value,layer))throw new IllegalStateException("Çap/eğim notu eklenemedi • katman "+layer);
                        applied++;break;
                    }
                    case "cad_insert_mechanical_block":{
                        String layer=requiredLayer(a);
                        String blockId=limitedString(a,"blockId",80);
                        String label=limitedString(a,"label",MAX_BLOCK_LABEL_CHARS);
                        double x=requiredFinite(a,"x"),y=requiredFinite(a,"y"),scale=requiredFinite(a,"scale"),rotation=requiredFinite(a,"rotation");
                        if(!cad.applyAiInsertLibraryBlock(blockId,x,y,scale,rotation,layer,label))
                            throw new IllegalStateException("Mekanik blok yerleştirilemedi • "+blockId);
                        applied++;break;
                    }
                    default:skipped++;break;
                }
            }
            int shown=highlights.isEmpty()?0:cad.setAiHighlightedSources(highlights);
            return new ApplyResult(true,false,applied,shown,skipped,
                "Gandalf önerileri uygulandı • "+applied+" çizim değişikliği"+(shown>0?" • "+shown+" nesne vurgulandı":"")+(skipped>0?" • "+skipped+" desteklenmeyen öneri atlandı":""));
        }catch(Exception e){
            cad.restoreCapturedSessionState(before);
            cad.clearAiHighlights();
            String reason=e.getMessage()==null?"Gandalf işlemi doğrulanamadı.":e.getMessage();
            return new ApplyResult(false,true,0,0,skipped,"Hiçbir Gandalf değişikliği kaydedilmedi. Toplu işlem geri alındı.\n"+reason);
        }
    }

    private static boolean isMutation(String name){
        return "cad_move_entity".equals(name)||"cad_delete_entity".equals(name)||"cad_change_layer".equals(name)||
            "cad_add_line".equals(name)||"cad_add_text".equals(name)||"cad_replace_text".equals(name)||
            "cad_add_polyline".equals(name)||"cad_offset_entity".equals(name)||"cad_trim_line".equals(name)||
            "cad_extend_line".equals(name)||"cad_continue_path".equals(name)||"cad_add_pipe_note".equals(name)||
            "cad_insert_mechanical_block".equals(name);
    }

    private static String label(String name){
        if("cad_highlight_entities".equals(name))return "Nesneleri vurgula";
        if("cad_move_entity".equals(name))return "Nesneyi taşı";
        if("cad_delete_entity".equals(name))return "Nesneyi sil";
        if("cad_change_layer".equals(name))return "Katmanı değiştir";
        if("cad_add_line".equals(name))return "Çizgi ekle";
        if("cad_add_text".equals(name))return "Metin/not ekle";
        if("cad_replace_text".equals(name))return "Metni değiştir";
        if("cad_add_polyline".equals(name))return "Polyline / hat ekle";
        if("cad_offset_entity".equals(name))return "Offset oluştur";
        if("cad_trim_line".equals(name))return "TRIM uygula";
        if("cad_extend_line".equals(name))return "EXTEND uygula";
        if("cad_continue_path".equals(name))return "Hattı devam ettir";
        if("cad_add_pipe_note".equals(name))return "Çap/eğim notu ekle";
        if("cad_insert_mechanical_block".equals(name))return "Mekanik blok yerleştir";
        return "Desteklenmeyen öneri: "+String.valueOf(name);
    }

    private static JSONObject parse(String raw)throws Exception{
        String text=raw==null?"{}":raw.trim();
        if(text.isEmpty())text="{}";
        return new JSONObject(text);
    }

    private static int requiredInt(JSONObject a,String key)throws Exception{
        if(a==null||!a.has(key))throw new IllegalArgumentException(key+" eksik");
        int id=a.getInt(key);if(id<0)throw new IllegalArgumentException("Geçersiz "+key);
        return id;
    }

    private static double[] requiredPoints(JSONObject a,String key,int minPoints,int maxPoints)throws Exception{
        JSONArray points=a==null?null:a.optJSONArray(key);
        if(points==null||points.length()<minPoints||points.length()>maxPoints)
            throw new IllegalArgumentException(key+" nokta sayısı geçersiz");
        double[] out=new double[points.length()*2];
        for(int i=0;i<points.length();i++){
            JSONArray point=points.optJSONArray(i);
            if(point==null||point.length()!=2)throw new IllegalArgumentException(key+"["+i+"] geçersiz");
            double x=point.getDouble(0),y=point.getDouble(1);
            if(!Double.isFinite(x)||!Double.isFinite(y)||Math.abs(x)>1e12d||Math.abs(y)>1e12d)
                throw new IllegalArgumentException(key+"["+i+"] koordinatı geçersiz");
            out[i*2]=x;out[i*2+1]=y;
        }
        return out;
    }

    private static String limitedString(JSONObject a,String key,int max)throws Exception{
        String value=a==null?"":a.optString(key,"").trim();
        if(value.length()>max)throw new IllegalArgumentException(key+" çok uzun");
        return value;
    }

    private static String pipeNote(String diameter,String slope){
        String d=diameter==null?"":diameter.trim(),s=slope==null?"":slope.trim();
        if(d.isEmpty())return s;
        if(s.isEmpty())return d;
        return d+" • "+s;
    }

    private static int requiredSourceId(JSONObject a)throws Exception{
        if(a==null||!a.has("sourceId"))throw new IllegalArgumentException("sourceId eksik");
        int id=a.getInt("sourceId");if(id<0)throw new IllegalArgumentException("Geçersiz sourceId");
        return id;
    }

    private static double requiredFinite(JSONObject a,String key)throws Exception{
        if(a==null||!a.has(key))throw new IllegalArgumentException(key+" eksik");
        double value=a.getDouble(key);
        if(!Double.isFinite(value)||Math.abs(value)>1e12d)throw new IllegalArgumentException("Geçersiz "+key);
        return value;
    }

    private static String requiredLayer(JSONObject a)throws Exception{
        String layer=a==null?"":a.optString("layer","").trim();
        if(layer.isEmpty()||layer.length()>MAX_LAYER_CHARS)throw new IllegalArgumentException("Geçersiz katman");
        return layer;
    }

    private static String requiredText(JSONObject a,String key)throws Exception{
        String value=a==null?"":a.optString(key,"");
        if(value.length()>MAX_TEXT_CHARS)throw new IllegalArgumentException("Metin güvenlik sınırını aşıyor");
        return value;
    }

    private static String limit(String value,int max){return value.length()<=max?value:value.substring(0,max-1)+"…";}
    private MusaAiActionExecutor(){}
}
