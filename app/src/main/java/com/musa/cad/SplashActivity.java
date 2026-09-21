package com.musa.cad;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.appcompat.app.AppCompatActivity;

public class SplashActivity extends AppCompatActivity {
    private Intent pendingIntent;

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        LockedScreenUi.enableImmersive(this);
        setContentView(R.layout.activity_splash);

        if(getApplication() instanceof MusaCadApp){
            ((MusaCadApp)getApplication()).markIntroSeen();
        }

        Intent incoming=new Intent(getIntent());
        boolean fileEntry=Intent.ACTION_VIEW.equals(incoming.getAction())
                ||Intent.ACTION_SEND.equals(incoming.getAction());
        pendingIntent=fileEntry?incoming:null;

        FrameLayout root=findViewById(R.id.splashRoot);
        FrameLayout stage=findViewById(R.id.artworkStage);

        LockedScreenUi.fillStage(this,root,stage,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_1);

            View.OnClickListener next=v->continueFlow();

            // Üstteki dört gerçek özellik kartı
            LockedScreenUi.hotspot(this,stage,81,430,192,205,next);
            LockedScreenUi.hotspot(this,stage,288,430,192,205,next);
            LockedScreenUi.hotspot(this,stage,497,430,192,205,next);
            LockedScreenUi.hotspot(this,stage,703,430,164,205,next);

            // Alttaki dört yüzer özellik düğmesi
            LockedScreenUi.hotspot(this,stage,49,1430,194,170,next);
            LockedScreenUi.hotspot(this,stage,263,1430,194,170,next);
            LockedScreenUi.hotspot(this,stage,478,1430,194,170,next);
            LockedScreenUi.hotspot(this,stage,694,1430,194,170,next);
        });
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    private void continueFlow(){
        if(pendingIntent!=null){
            if(LicenseManager.hasAccess(this)){
                Intent next=new Intent(pendingIntent).setClass(this,MainActivity.class);
                next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                startActivity(next);
            }else{
                Intent next=new Intent(this,LicenseActivity.class);
                next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,pendingIntent);
                startActivity(next);
            }
        }else{
            startActivity(new Intent(this,AboutActivity.class));
        }
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }
}
