package com.musa.cad;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Stores only MusaCAD's cloud-access state.
 *
 * No OpenAI API key or bearer token is stored in the APK/preferences.
 * Account linkage means only that a trusted MusaCAD gateway authenticated the
 * session. ChatGPT identity alone must never be treated as Plus/Pro proof.
 */
public final class MusaAiCloudAccess {
    private static final String PREFS="musa_ai_cloud_access_v1";
    private static final String K_LINKED="cloud_account_linked";
    private static final String K_LEGACY_LINKED="chatgpt_identity_linked";
    private static final String K_VERIFIED="cloud_entitlement_verified";
    private static final String K_EXPIRES="cloud_entitlement_expires";
    private static final String K_SOURCE="cloud_entitlement_source";

    public static MusaAiAccessPolicy.Decision decision(Context context){
        SharedPreferences p=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
        boolean linked=p.getBoolean(K_LINKED,p.getBoolean(K_LEGACY_LINKED,false));
        boolean verified=p.getBoolean(K_VERIFIED,false);
        long expires=p.getLong(K_EXPIRES,0L);
        boolean valid=verified&&expires>System.currentTimeMillis()&&MusaAiCloudSession.isValid();
        boolean gateway=BuildConfig.AI_GATEWAY_URL!=null&&!BuildConfig.AI_GATEWAY_URL.trim().isEmpty();
        return MusaAiAccessPolicy.decide(gateway,linked,verified,valid);
    }

    public static boolean isAccountLinked(Context context){
        SharedPreferences p=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
        return p.getBoolean(K_LINKED,p.getBoolean(K_LEGACY_LINKED,false));
    }

    public static void setAccountLinked(Context context,boolean linked){
        SharedPreferences.Editor e=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit()
            .putBoolean(K_LINKED,linked).remove(K_LEGACY_LINKED);
        if(!linked){
            e.putBoolean(K_VERIFIED,false).putLong(K_EXPIRES,0L).remove(K_SOURCE);
            MusaAiCloudSession.clear();
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
        MusaAiCloudSession.clear();
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().clear().apply();
    }

    private MusaAiCloudAccess(){}
}
