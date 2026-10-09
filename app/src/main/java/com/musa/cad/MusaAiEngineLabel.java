package com.musa.cad;

import java.util.Locale;

/** Display only model identity actually returned by the authenticated AI gateway. */
public final class MusaAiEngineLabel {
    public static String display(String provider,String model){
        String p=provider==null?"":provider.trim().toLowerCase(Locale.ROOT);
        String name;
        switch(p){
            case "cloudflare": name="Qwen (Cloudflare Workers AI)";break;
            case "gemini": name="Gemini";break;
            case "selfhosted": name="Qwen / özel sunucu";break;
            case "local": name="Yerel CAD motoru (görsel AI değil)";break;
            case "": name="Sağlayıcı doğrulanmadı";break;
            default: name="Bulut AI ("+p.replaceAll("[^a-z0-9_.-]","")+")";break;
        }
        String m=model==null?"":model.replaceAll("[\\r\\n]"," ").trim();
        if(m.length()>72)m=m.substring(0,72);
        return m.isEmpty()?name:name+" • "+m;
    }
    private MusaAiEngineLabel(){}
}
