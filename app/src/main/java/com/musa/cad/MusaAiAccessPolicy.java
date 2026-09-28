package com.musa.cad;

/**
 * Pure access-decision policy for MusaCAD AI.
 *
 * Important: a ChatGPT identity login is not proof of a Plus/Pro subscription.
 * Cloud mode is enabled only after a trusted backend explicitly verifies
 * cloud entitlement and the session is still valid.
 */
public final class MusaAiAccessPolicy {
    public enum Mode { LOCAL_LIMITED, CLOUD_GANDALF }

    public static final class Decision {
        public final Mode mode;
        public final String badge;
        public final String reason;
        Decision(Mode mode,String badge,String reason){
            this.mode=mode;this.badge=badge;this.reason=reason;
        }
        public boolean cloud(){return mode==Mode.CLOUD_GANDALF;}
    }

    public static Decision decide(boolean gatewayConfigured,
                                  boolean chatGptIdentityLinked,
                                  boolean cloudEntitlementVerified,
                                  boolean sessionValid){
        if(!gatewayConfigured){
            return local("Yerel • Kısıtlı","Çevrimiçi Gandalf ağ geçidi yapılandırılmamış.");
        }
        if(!chatGptIdentityLinked){
            return local("Yerel • Kısıtlı","ChatGPT hesabı bağlı değil.");
        }
        if(!cloudEntitlementVerified){
            return local("Yerel • Kısıtlı","Çevrimiçi AI yetkisi doğrulanmadı.");
        }
        if(!sessionValid){
            return local("Yerel • Kısıtlı","Çevrimiçi AI oturumu geçerli değil.");
        }
        return new Decision(Mode.CLOUD_GANDALF,"Gandalf • Çevrimiçi",
            "Doğrulanmış çevrimiçi AI yetkisi aktif.");
    }

    private static Decision local(String badge,String reason){
        return new Decision(Mode.LOCAL_LIMITED,badge,reason);
    }

    private MusaAiAccessPolicy(){}
}
