package com.musa.cad;

/** Process-memory only cloud session. Never persists bearer tokens to disk. */
public final class MusaAiCloudSession {
    private static volatile String bearerToken="";
    private static volatile long expiresAtMs=0L;

    public static synchronized void set(String token,long expiresEpochMs){
        bearerToken=token==null?"":token.trim();
        expiresAtMs=bearerToken.isEmpty()?0L:Math.max(0L,expiresEpochMs);
    }

    public static synchronized void clear(){
        bearerToken="";
        expiresAtMs=0L;
    }

    public static boolean isValid(){
        String t=bearerToken;
        return t!=null&&!t.isEmpty()&&expiresAtMs>System.currentTimeMillis();
    }

    public static String token(){
        return isValid()?bearerToken:"";
    }

    private MusaAiCloudSession(){}
}
