package com.musa.cad;

import android.content.Context;
import android.content.SharedPreferences;
import java.util.Locale;

public final class LicenseManager {
    private static final String PREFS="musacad_license_state";
    private static final String K_TRIAL_START="trial_start_v1";
    private static final String K_LAST_SEEN="last_seen_v1";
    private static final String K_TERMS_VERSION="terms_version";
    private static final String K_LICENSED="licensed_v1";
    public static final int TERMS_VERSION=1;

    public enum State { TRIAL_AVAILABLE, TRIAL_ACTIVE, TRIAL_EXPIRED, LICENSED, CLOCK_ERROR }
    public enum ActivationResult { ACTIVATED, INVALID_CODE, NOT_CONFIGURED }

    private static SharedPreferences prefs(Context c){
        return c.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
    }

    public static State state(Context c){
        SharedPreferences p=prefs(c);
        if(p.getBoolean(K_LICENSED,false))return State.LICENSED;
        long start=p.getLong(K_TRIAL_START,0L);
        if(start<=0)return State.TRIAL_AVAILABLE;
        long last=p.getLong(K_LAST_SEEN,0L),now=System.currentTimeMillis();
        if(last>0&&now+TrialPolicy.CLOCK_ROLLBACK_TOLERANCE_MS<last)return State.CLOCK_ERROR;
        return TrialPolicy.isActive(start,last,now)?State.TRIAL_ACTIVE:State.TRIAL_EXPIRED;
    }

    public static boolean hasAccess(Context c){
        State s=state(c);
        if(s==State.LICENSED)return true;
        if(s==State.TRIAL_ACTIVE){touch(c);return true;}
        return false;
    }

    public static boolean termsAccepted(Context c){
        return prefs(c).getInt(K_TERMS_VERSION,0)>=TERMS_VERSION;
    }

    public static void acceptTerms(Context c){
        prefs(c).edit().putInt(K_TERMS_VERSION,TERMS_VERSION).apply();
    }

    public static boolean startTrial(Context c){
        if(!termsAccepted(c))return false;
        SharedPreferences p=prefs(c);
        if(p.getLong(K_TRIAL_START,0L)>0)return state(c)==State.TRIAL_ACTIVE;
        long now=System.currentTimeMillis();
        p.edit().putLong(K_TRIAL_START,now).putLong(K_LAST_SEEN,now).apply();
        return true;
    }

    public static long remainingMs(Context c){
        SharedPreferences p=prefs(c);
        return TrialPolicy.remainingMs(p.getLong(K_TRIAL_START,0L),p.getLong(K_LAST_SEEN,0L),System.currentTimeMillis());
    }

    public static String remainingLabel(Context c){
        long ms=remainingMs(c);
        if(ms<=0)return "Deneme süresi sona erdi";
        long minutes=(ms+59999L)/60000L;
        long hours=minutes/60L,mins=minutes%60L;
        return hours>0?String.format(Locale.getDefault(),"Deneme: %d sa %d dk kaldı",hours,mins):String.format(Locale.getDefault(),"Deneme: %d dk kaldı",mins);
    }

    /**
     * The UI and storage contract are ready, but commercial key verification deliberately has no
     * hard-coded master key. The separate license-generator step will add public-key verification;
     * only the public key belongs in the APK, never the private signing key.
     */
    public static ActivationResult activateCode(Context c,String code){
        if(code==null||code.trim().isEmpty())return ActivationResult.INVALID_CODE;
        return ActivationResult.NOT_CONFIGURED;
    }

    private static void touch(Context c){
        long now=System.currentTimeMillis();
        SharedPreferences p=prefs(c);
        long last=p.getLong(K_LAST_SEEN,0L);
        if(now>last)p.edit().putLong(K_LAST_SEEN,now).apply();
    }

    private LicenseManager(){}
}
