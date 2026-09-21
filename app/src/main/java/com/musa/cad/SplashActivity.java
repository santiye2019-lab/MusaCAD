package com.musa.cad;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;

public class SplashActivity extends AppCompatActivity {
    private Intent pendingIntent;
    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_splash);
        if(getApplication() instanceof MusaCadApp)((MusaCadApp)getApplication()).markIntroSeen();
        Intent incoming=new Intent(getIntent());
        boolean fileEntry=Intent.ACTION_VIEW.equals(incoming.getAction())||Intent.ACTION_SEND.equals(incoming.getAction());
        pendingIntent=fileEntry?incoming:null;

        FrameLayout root=findViewById(R.id.splashRoot), stage=findViewById(R.id.artworkStage);
        LockedScreenUi.fitStage(this,root,stage,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_1);
            View.OnClickListener enter=v->openApp();
            LockedScreenUi.hotspot(this,stage,104,451,168,160,enter);
            LockedScreenUi.hotspot(this,stage,293,451,168,160,enter);
            LockedScreenUi.hotspot(this,stage,484,451,171,160,enter);
            LockedScreenUi.hotspot(this,stage,677,451,168,160,enter);
            LockedScreenUi.hotspot(this,stage,52,1460,190,150,enter);
            LockedScreenUi.hotspot(this,stage,263,1460,190,150,enter);
            LockedScreenUi.hotspot(this,stage,474,1460,190,150,enter);
            LockedScreenUi.hotspot(this,stage,685,1460,190,150,enter);
        });
    }
    private void openApp(){
        if(LicenseManager.hasAccess(this)){
            Intent next=pendingIntent==null?new Intent(this,MainActivity.class):new Intent(pendingIntent).setClass(this,MainActivity.class);
            next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);startActivity(next);
        }else{
            Intent next=new Intent(this,LicenseActivity.class);
            if(pendingIntent!=null)next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,pendingIntent);
            startActivity(next);
        }
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
    }
}
