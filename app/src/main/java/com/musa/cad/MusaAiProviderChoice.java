package com.musa.cad;

import android.content.Context;

/**
 * User-owned, device-private model selection. No provider keys or upstream URLs
 * are stored on the phone; the authenticated Worker chooses server credentials.
 */
public final class MusaAiProviderChoice {
    private static final String PREFS="musacad_ai_provider_choice";
    private static final String KEY="provider";
    public static final String DEFAULT="";
    public static final String GEMINI="gemini";
    public static final String QWEN="cloudflare";

    public static String normalize(String value){
        return GEMINI.equals(value)||QWEN.equals(value)?value:DEFAULT;
    }
    public static String selected(Context context){
        if(context==null)return DEFAULT;
        return normalize(context.getApplicationContext()
            .getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(KEY,DEFAULT));
    }
    public static void select(Context context,String provider){
        if(context==null)return;
        context.getApplicationContext().getSharedPreferences(PREFS,Context.MODE_PRIVATE)
            .edit().putString(KEY,normalize(provider)).apply();
    }
    public static String label(String provider){
        switch(normalize(provider)){
            case GEMINI:return "Gemini";
            case QWEN:return "Qwen (Cloudflare)";
            default:return "Sunucu varsayılanı";
        }
    }
    private MusaAiProviderChoice(){}
}
