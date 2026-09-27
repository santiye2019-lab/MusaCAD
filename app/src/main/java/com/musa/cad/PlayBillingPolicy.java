package com.musa.cad;

/** Commercial-policy guard for the Google Play yearly MusaCAD renewal product. */
public final class PlayBillingPolicy {
    public static final String YEARLY_BILLING_PERIOD="P1Y";

    public static boolean isYearlyBillingPeriod(String billingPeriod){
        return billingPeriod!=null&&YEARLY_BILLING_PERIOD.equalsIgnoreCase(billingPeriod.trim());
    }

    private PlayBillingPolicy(){}
}
