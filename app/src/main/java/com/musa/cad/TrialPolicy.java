package com.musa.cad;

/** Pure trial-time policy so the commercial access rules can be regression-tested without Android. */
public final class TrialPolicy {
    public static final long TRIAL_DURATION_MS=24L*60L*60L*1000L;
    public static final long CLOCK_ROLLBACK_TOLERANCE_MS=5L*60L*1000L;

    public static boolean isActive(long startedAt,long lastSeen,long now){
        if(startedAt<=0||now<startedAt-CLOCK_ROLLBACK_TOLERANCE_MS)return false;
        if(lastSeen>0&&now+CLOCK_ROLLBACK_TOLERANCE_MS<lastSeen)return false;
        return now-startedAt<TRIAL_DURATION_MS;
    }

    public static long remainingMs(long startedAt,long lastSeen,long now){
        if(!isActive(startedAt,lastSeen,now))return 0L;
        return Math.max(0L,TRIAL_DURATION_MS-(now-startedAt));
    }

    private TrialPolicy(){}
}
