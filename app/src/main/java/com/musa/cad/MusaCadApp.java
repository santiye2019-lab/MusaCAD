package com.musa.cad;

import android.app.*;
import android.content.Intent;
import android.os.Bundle;

/** Application-level guard so an expired trial cannot be bypassed by resuming an old MainActivity. */
public class MusaCadApp extends Application implements Application.ActivityLifecycleCallbacks {
    private boolean redirecting;
    private boolean introSeenThisProcess;
    private PlayBillingManager playBilling;
    private Activity resumedActivity;

    public void markIntroSeen(){introSeenThisProcess=true;}
    public boolean introSeenThisProcess(){return introSeenThisProcess;}

    @Override public void onCreate(){
        super.onCreate();
        registerActivityLifecycleCallbacks(this);
        playBilling=new PlayBillingManager(this,new PlayBillingManager.Listener(){
            @Override public void onProductReady(boolean ready,String displayPrice){}
            @Override public void onEntitlementChanged(boolean active){
                if(!active)enforceAccessIfNeeded();
            }
            @Override public void onBillingMessage(String message){}
        });
        playBilling.start();
    }

    @Override public void onActivityResumed(Activity activity){
        resumedActivity=activity;
        if(playBilling!=null)playBilling.refresh();
        if(!(activity instanceof MainActivity)||redirecting)return;
        if(!introSeenThisProcess){
            redirecting=true;
            Intent splash=new Intent(activity,SplashActivity.class);
            splash.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            activity.startActivity(splash);
            activity.finish();
            redirecting=false;
            return;
        }
        if(LicenseManager.hasAccess(activity))return;
        redirecting=true;
        Intent gate=new Intent(activity,LicenseActivity.class);
        activity.startActivity(gate);
        activity.finish();
        redirecting=false;
    }

    private void enforceAccessIfNeeded(){
        Activity activity=resumedActivity;
        if(!(activity instanceof MainActivity)||redirecting||LicenseManager.hasAccess(activity))return;
        activity.runOnUiThread(()->{
            if(redirecting||LicenseManager.hasAccess(activity))return;
            redirecting=true;
            Intent gate=new Intent(activity,LicenseActivity.class);
            activity.startActivity(gate);
            activity.finish();
            redirecting=false;
        });
    }

    @Override public void onActivityCreated(Activity a,Bundle b){}
    @Override public void onActivityStarted(Activity a){}
    @Override public void onActivityPaused(Activity a){if(resumedActivity==a)resumedActivity=null;}
    @Override public void onActivityStopped(Activity a){}
    @Override public void onActivitySaveInstanceState(Activity a,Bundle b){}
    @Override public void onActivityDestroyed(Activity a){}

    @Override public void onTerminate(){
        if(playBilling!=null)playBilling.close();
        super.onTerminate();
    }
}
