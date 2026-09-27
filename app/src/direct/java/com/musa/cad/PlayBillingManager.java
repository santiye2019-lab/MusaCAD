package com.musa.cad;

import android.app.Activity;
import android.content.Context;

/**
 * Direct-APK / institutional distribution stub.
 * This variant intentionally has no Google Play Billing dependency or purchase path.
 */
public final class PlayBillingManager {
    public interface Listener {
        void onProductReady(boolean ready,String displayPrice);
        void onEntitlementChanged(boolean active);
        void onBillingMessage(String message);
    }

    private final Listener listener;

    public PlayBillingManager(Context context,Listener listener){
        this.listener=listener;
    }

    public void start(){
        if(listener!=null)listener.onProductReady(false,"");
    }

    public void refresh(){
        if(listener!=null)listener.onProductReady(false,"");
    }

    public void launchPurchase(Activity activity){
        if(listener!=null)listener.onBillingMessage(
            "Bu MusaCAD sürümü doğrudan APK / kurumsal dağıtımdır. Lisans için Serial + 12 haneli lisans kodunu kullanın."
        );
    }

    public void close(){
        // No Play Billing connection exists in the direct distribution.
    }
}
