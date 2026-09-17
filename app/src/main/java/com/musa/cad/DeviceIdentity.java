package com.musa.cad;

import android.content.Context;
import android.provider.Settings;
import java.util.Locale;
import java.util.UUID;

/** Android source for a stable, privacy-preserving MusaCAD license identifier. */
public final class DeviceIdentity {
    private static final String PREFS="musacad_license_state";
    private static final String K_FALLBACK="device_identity_fallback_v2";

    public static String licenseId(Context context){
        if(context==null)return "";
        String androidId=Settings.Secure.getString(context.getContentResolver(),Settings.Secure.ANDROID_ID);
        String stable=DeviceIdentityHash.derive(androidId,context.getPackageName());
        if(!stable.isEmpty())return stable;
        String fallback=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(K_FALLBACK,null);
        if(fallback!=null&&!fallback.isEmpty())return fallback;
        fallback="MC-FALLBACK-"+UUID.randomUUID().toString().toUpperCase(Locale.ROOT);
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().putString(K_FALLBACK,fallback).commit();
        return fallback;
    }
    private DeviceIdentity(){}
}
