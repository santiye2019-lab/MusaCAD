package com.musa.cad;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Stores only MusaCAD's cloud-access state.
 *
 * No OpenAI API key is stored in the APK or preferences. A future ChatGPT
 * sign-in flow may mark identity linkage, while a trusted MusaCAD backend is
 * responsible for verifying any cloud entitlement.
 */
public final class MusaAiCloudAccess {
    private static final String PREFS="musa_ai_cloud_access_v1";
    private static final String K_LINKED="chatgpt_identity_linked";
    private static final String K_VERIFIED="cloud_entitlement_verified";
    private static final String K_EXPIRES="cloud_entitlement_expires";
    private static final String K_SOURCE="cloud_entitlement_source";

    public static MusaAiAccessPolicy.Decision decision(Context context){
        SharedPreferences p=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
        boolean linked=p.getBoolean(K_LINKED,false);
        boolean verified=p.getBoolean(K_VERIFIED,false);
        long expires=p.getLong(K_EXPIRES,0L);
        boolean valid=verified&&expires>System.currentTimeMillis();
        boolean gateway=BuildConfig.AI_GATEWAY_URL!=null&&!BuildConfig.AI_GATEWAY_URL.trim().isEmpty();
        return MusaAiAccessPolicy.decide(gateway,linked,verified,valid);
    }

    public static boolean isChatGptIdentityLinked(Context context){
        return context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getBoolean(K_LINKED,false);
    }

    public static void setChatGptIdentityLinked(Context context,boolean linked){
        SharedPreferences.Editor e=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit()
            .putBoolean(K_LINKED,linked);
        if(!linked){
            e.putBoolean(K_VERIFIED,false).putLong(K_EXPIRES,0L).remove(K_SOURCE);
        }
        e.apply();
    }

    /**
     * Called only with a trusted backend result. Identity sign-in alone must
     * never call this as proof of ChatGPT Plus/Pro.
     */
    public static void setVerifiedCloudEntitlement(Context context,boolean verified,long expiresEpochMs,String source){
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit()
            .putBoolean(K_VERIFIED,verified)
            .putLong(K_EXPIRES,verified?Math.max(0L,expiresEpochMs):0L)
            .putString(K_SOURCE,source==null?"":source)
            .apply();
    }

    public static void clear(Context context){
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().clear().apply();
    }

    private MusaAiCloudAccess(){}
}
