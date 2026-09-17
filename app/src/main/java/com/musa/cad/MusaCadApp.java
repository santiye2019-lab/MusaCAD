package com.musa.cad;

import android.app.*;
import android.content.Intent;
import android.os.Bundle;

/** Application-level guard so an expired trial cannot be bypassed by resuming an old MainActivity. */
public class MusaCadApp extends Application implements Application.ActivityLifecycleCallbacks {
    private boolean redirecting;

    @Override public void onCreate(){
        super.onCreate();registerActivityLifecycleCallbacks(this);
    }

    @Override public void onActivityResumed(Activity activity){
        if(!(activity instanceof MainActivity)||redirecting)return;
        if(LicenseManager.hasAccess(activity))return;
        redirecting=true;
        Intent gate=new Intent(activity,LicenseActivity.class);
        activity.startActivity(gate);
        activity.finish();
        redirecting=false;
    }

    @Override public void onActivityCreated(Activity a,Bundle b){}
    @Override public void onActivityStarted(Activity a){}
    @Override public void onActivityPaused(Activity a){}
    @Override public void onActivityStopped(Activity a){}
    @Override public void onActivitySaveInstanceState(Activity a,Bundle b){}
    @Override public void onActivityDestroyed(Activity a){}
}
