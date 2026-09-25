package com.musa.cad;

import android.content.Context;
import android.content.SharedPreferences;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

public final class LicenseManager {
    private static final String PREFS="musacad_license_state";
    private static final String K_TRIAL_START="trial_start_v1";
    private static final String K_LAST_SEEN="last_seen_v1";
    private static final String K_TERMS_VERSION="terms_version";
    private static final String K_LICENSE_TOKEN="license_token_v1";
    private static final String K_TRIAL_TOKEN="trial_token_v2";
    private static final String K_SERVER_TRIAL_USED="server_trial_used_v2";
    private static final String K_SIGNED_TRIAL_LAST_SEEN="signed_trial_last_seen_v2";
    private static final String K_PLAY_ENTITLED="play_entitled_v1";
    private static final String K_EVER_PAID_LICENSE="ever_paid_license_v1";
    public static final int TERMS_VERSION=1;

    public enum State { TRIAL_AVAILABLE, TRIAL_ACTIVE, TRIAL_EXPIRED, LICENSED, CLOCK_ERROR }
    public enum ActivationResult { ACTIVATED, INVALID_CODE }

    private static SharedPreferences prefs(Context c){return c.getSharedPreferences(PREFS,Context.MODE_PRIVATE);}

    public static State state(Context c){
        SharedPreferences p=prefs(c);
        String paid=p.getString(K_LICENSE_TOKEN,null);
        if(paid!=null){
            // Any stored paid token was accepted by activateCode() when it was written,
            // so it is safe to remember that this device has had a paid MusaCAD license.
            p.edit().putBoolean(K_EVER_PAID_LICENSE,true).apply();
            if(verifyStoredPaidToken(c,paid))return State.LICENSED;
        }
        if(p.getBoolean(K_PLAY_ENTITLED,false)){
            p.edit().putBoolean(K_EVER_PAID_LICENSE,true).apply();
            return State.LICENSED;
        }

        String signedTrial=p.getString(K_TRIAL_TOKEN,null);
        if(signedTrial!=null){
            long now=System.currentTimeMillis(),last=p.getLong(K_SIGNED_TRIAL_LAST_SEEN,0L);
            if(!SignedTrialPolicy.clockOk(last,now))return State.CLOCK_ERROR;
            TrialToken.Result r=verifyTrialToken(c,signedTrial,now);
            if(r!=null&&SignedTrialPolicy.validWindow(r.valid,r.expiresAtMs,now)){
                if(now>last)p.edit().putLong(K_SIGNED_TRIAL_LAST_SEEN,now).apply();
                return State.TRIAL_ACTIVE;
            }
            p.edit().remove(K_TRIAL_TOKEN).remove(K_SIGNED_TRIAL_LAST_SEEN).putBoolean(K_SERVER_TRIAL_USED,true).apply();
            return State.TRIAL_EXPIRED;
        }
        if(p.getBoolean(K_SERVER_TRIAL_USED,false))return State.TRIAL_EXPIRED;

        // Legacy pre-server trial state is retained only for development/migration compatibility.
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

    public static boolean termsAccepted(Context c){return prefs(c).getInt(K_TERMS_VERSION,0)>=TERMS_VERSION;}
    public static void acceptTerms(Context c){prefs(c).edit().putInt(K_TERMS_VERSION,TERMS_VERSION).apply();}

    /** Legacy local trial start retained for old development installs; production UI uses TrialService. */
    public static boolean startTrial(Context c){
        if(!termsAccepted(c))return false;
        SharedPreferences p=prefs(c);
        if(p.getBoolean(K_SERVER_TRIAL_USED,false)||p.getString(K_TRIAL_TOKEN,null)!=null)return state(c)==State.TRIAL_ACTIVE;
        if(p.getLong(K_TRIAL_START,0L)>0)return state(c)==State.TRIAL_ACTIVE;
        long now=System.currentTimeMillis();
        p.edit().putLong(K_TRIAL_START,now).putLong(K_LAST_SEEN,now).apply();
        return true;
    }

    /** Accepts only a finite signed token whose expiry is approximately one day from activation. */
    public static boolean activateTrialToken(Context c,String token){
        if(token==null||token.trim().isEmpty())return false;
        long now=System.currentTimeMillis();TrialToken.Result r=verifyTrialToken(c,token,now);
        if(r==null||!SignedTrialPolicy.validWindow(r.valid,r.expiresAtMs,now))return false;
        prefs(c).edit().putString(K_TRIAL_TOKEN,token.trim()).putLong(K_SIGNED_TRIAL_LAST_SEEN,now).putBoolean(K_SERVER_TRIAL_USED,true).remove(K_TRIAL_START).remove(K_LAST_SEEN).commit();
        return true;
    }

    public static void markServerTrialUsed(Context c){prefs(c).edit().putBoolean(K_SERVER_TRIAL_USED,true).remove(K_TRIAL_TOKEN).remove(K_SIGNED_TRIAL_LAST_SEEN).apply();}

    public static long remainingMs(Context c){
        SharedPreferences p=prefs(c);String signedTrial=p.getString(K_TRIAL_TOKEN,null);long now=System.currentTimeMillis();
        if(signedTrial!=null){long last=p.getLong(K_SIGNED_TRIAL_LAST_SEEN,0L);TrialToken.Result r=verifyTrialToken(c,signedTrial,now);return r==null?0L:SignedTrialPolicy.remainingMs(r.valid,r.expiresAtMs,last,now);}
        return TrialPolicy.remainingMs(p.getLong(K_TRIAL_START,0L),p.getLong(K_LAST_SEEN,0L),now);
    }

    public static String remainingLabel(Context c){
        long ms=remainingMs(c);if(ms<=0)return "Deneme süresi sona erdi";
        long minutes=(ms+59999L)/60000L,hours=minutes/60L,mins=minutes%60L;
        return hours>0?String.format(Locale.getDefault(),"Deneme: %d sa %d dk kaldı",hours,mins):String.format(Locale.getDefault(),"Deneme: %d dk kaldı",mins);
    }

    /** Stable on normal reinstall when Android supplies the same app-scoped ANDROID_ID. */
    public static String installationId(Context c){return DeviceIdentity.licenseId(c);}

    /** Cached Google Play ownership, refreshed from Play Billing when the app process starts. */
    public static void setPlayEntitlement(Context c,boolean active){
        SharedPreferences.Editor e=prefs(c).edit().putBoolean(K_PLAY_ENTITLED,active);
        if(active)e.putBoolean(K_EVER_PAID_LICENSE,true);
        e.apply();
    }

    /** Google Play is intentionally restricted to yearly renewal of a previously paid MusaCAD license. */
    public static boolean eligibleForPlayYearlyRenewal(Context c){
        SharedPreferences p=prefs(c);
        return p.getBoolean(K_EVER_PAID_LICENSE,false)
            || p.getBoolean(K_PLAY_ENTITLED,false)
            || p.getString(K_LICENSE_TOKEN,null)!=null;
    }

    public static boolean hasPlayEntitlement(Context c){
        return prefs(c).getBoolean(K_PLAY_ENTITLED,false);
    }

    public static ActivationResult activateCode(Context c,String code){
        if(code==null||code.trim().isEmpty())return ActivationResult.INVALID_CODE;
        try{LicenseToken.Result r=verifyPaidToken(c,code,System.currentTimeMillis());if(r==null||!r.valid)return ActivationResult.INVALID_CODE;prefs(c).edit().putString(K_LICENSE_TOKEN,code.trim()).putBoolean(K_EVER_PAID_LICENSE,true).commit();return ActivationResult.ACTIVATED;}
        catch(Exception e){return ActivationResult.INVALID_CODE;}
    }

    private static boolean verifyStoredPaidToken(Context c,String token){LicenseToken.Result r=verifyPaidToken(c,token,System.currentTimeMillis());if(r!=null&&r.valid)return true;prefs(c).edit().remove(K_LICENSE_TOKEN).apply();return false;}

    private static LicenseToken.Result verifyPaidToken(Context c,String token,long now){
        try{return LicenseToken.verify(token,installationId(c),now,readAsset(c,"MUSACAD-LICENSE-PUBLIC.pem"));}
        catch(Exception e){return null;}
    }

    private static TrialToken.Result verifyTrialToken(Context c,String token,long now){
        try{
            String publicKey=BuildConfig.TRIAL_PUBLIC_KEY_PEM==null?"":BuildConfig.TRIAL_PUBLIC_KEY_PEM.trim();
            if(publicKey.isEmpty())return null;
            return TrialToken.verify(token,installationId(c),now,publicKey);
        }catch(Exception e){return null;}
    }

    private static String readAsset(Context c,String name)throws Exception{
        try(InputStream in=c.getAssets().open(name);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];int n;while((n=in.read(b))!=-1)out.write(b,0,n);return out.toString(StandardCharsets.UTF_8.name());
        }
    }

    private static void touch(Context c){
        long now=System.currentTimeMillis();SharedPreferences p=prefs(c);
        if(p.getString(K_TRIAL_TOKEN,null)!=null){long last=p.getLong(K_SIGNED_TRIAL_LAST_SEEN,0L);if(now>last)p.edit().putLong(K_SIGNED_TRIAL_LAST_SEEN,now).apply();return;}
        long last=p.getLong(K_LAST_SEEN,0L);if(now>last)p.edit().putLong(K_LAST_SEEN,now).apply();
    }
    private LicenseManager(){}
}
