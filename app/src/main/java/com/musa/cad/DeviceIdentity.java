package com.musa.cad;

import android.content.Context;
import android.provider.Settings;

/** Android source for a stable, privacy-preserving MusaCAD license identifier. */
public final class DeviceIdentity {
    public static String licenseId(Context context){
        if(context==null)return "";
        String androidId=Settings.Secure.getString(context.getContentResolver(),Settings.Secure.ANDROID_ID);
        return DeviceIdentityHash.derive(androidId,context.getPackageName());
    }
    private DeviceIdentity(){}
}
