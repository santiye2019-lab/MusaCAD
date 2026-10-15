package com.musa.cad;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Bounded, device-private named UCS preset codec. No Android dependency; malformed
 * stored preferences never silently switch the coordinate system or crash CAD.
 */
public final class CadUcsProfiles {
    public static final int MAX_PROFILES=12;
    private static final int MAX_STORED_CHARS=8192;
    private CadUcsProfiles(){}

    public static String validName(String name){
        if(name==null)throw new IllegalArgumentException("UCS adı boş olamaz");
        String cleaned=name.trim();
        if(cleaned.isEmpty()||cleaned.length()>32||
           !cleaned.matches("[\\p{L}\\p{N} _.-]+"))
            throw new IllegalArgumentException("UCS adı 1–32 harf, sayı, boşluk, nokta, alt çizgi veya tire içermeli");
        return cleaned;
    }

    public static Map<String,CadUcsFrame> read(String stored){
        LinkedHashMap<String,CadUcsFrame> result=new LinkedHashMap<>();
        if(stored==null||stored.isEmpty()||stored.length()>MAX_STORED_CHARS)
            return Collections.unmodifiableMap(result);
        for(String line:stored.split("\n",-1)){
            if(line.isEmpty())continue;
            int tab=line.indexOf('\t');
            if(tab<=0||tab!=line.lastIndexOf('\t')||result.size()>=MAX_PROFILES)
                continue;
            try{
                String name=validName(line.substring(0,tab));
                if(!result.containsKey(name))
                    result.put(name,CadUcsFrame.parse(line.substring(tab+1)));
            }catch(IllegalArgumentException ignored){
                // A corrupt/old record must not prevent opening other valid presets.
            }
        }
        return Collections.unmodifiableMap(result);
    }

    public static String put(String stored,String name,CadUcsFrame frame){
        name=validName(name);
        if(frame==null)throw new IllegalArgumentException("Kaydedilecek UCS bulunamadı");
        LinkedHashMap<String,CadUcsFrame> entries=new LinkedHashMap<>(read(stored));
        if(entries.size()>=MAX_PROFILES&&!entries.containsKey(name))
            throw new IllegalArgumentException("En fazla "+MAX_PROFILES+" UCS kaydedilebilir");
        entries.put(name,frame);
        return write(entries);
    }

    public static String remove(String stored,String name){
        name=validName(name);
        LinkedHashMap<String,CadUcsFrame> entries=new LinkedHashMap<>(read(stored));
        entries.remove(name);
        return write(entries);
    }

    private static String write(Map<String,CadUcsFrame> entries){
        StringBuilder out=new StringBuilder();
        for(Map.Entry<String,CadUcsFrame> entry:entries.entrySet()){
            String name=validName(entry.getKey());
            CadUcsFrame frame=entry.getValue();
            double[] o=frame.origin(),x=frame.xAxis(),y=frame.yAxis();
            out.append(name).append('\t');
            for(double value:o)out.append(Double.toString(value)).append(',');
            for(double value:x)out.append(Double.toString(value)).append(',');
            for(int i=0;i<3;i++){
                if(i>0)out.append(',');
                out.append(Double.toString(y[i]));
            }
            out.append('\n');
        }
        if(out.length()>MAX_STORED_CHARS)
            throw new IllegalArgumentException("Kayıtlı UCS veri sınırı aşıldı");
        return out.toString();
    }
}
