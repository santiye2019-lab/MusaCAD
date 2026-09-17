package com.musa.cad;

/** Pure policy for server-issued one-day signed trials. */
public final class SignedTrialPolicy {
    public static final long SERVER_CLOCK_TOLERANCE_MS=5L*60L*1000L;

    public static boolean clockOk(long lastSeen,long now){return lastSeen<=0L||now+TrialPolicy.CLOCK_ROLLBACK_TOLERANCE_MS>=lastSeen;}

    public static boolean validWindow(boolean tokenValid,long expiresAtMs,long now){
        return tokenValid&&expiresAtMs>now&&expiresAtMs<=now+TrialPolicy.TRIAL_DURATION_MS+SERVER_CLOCK_TOLERANCE_MS;
    }

    public static long remainingMs(boolean tokenValid,long expiresAtMs,long lastSeen,long now){
        if(!clockOk(lastSeen,now)||!validWindow(tokenValid,expiresAtMs,now))return 0L;
        return Math.max(0L,expiresAtMs-now);
    }
    private SignedTrialPolicy(){}
}
