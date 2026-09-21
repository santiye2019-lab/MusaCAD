package com.musa.cad;

import android.content.Intent;
import android.os.Bundle;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.appcompat.app.AppCompatActivity;

public class AboutActivity extends AppCompatActivity {
    public static final String EXTRA_CONTINUE_TO_APP="com.musa.cad.CONTINUE_TO_APP";

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        LockedScreenUi.enableImmersive(this);
        setContentView(R.layout.activity_about);

        FrameLayout root=findViewById(R.id.aboutRoot);
        FrameLayout stage=findViewById(R.id.artworkStage);

        LockedScreenUi.fillStage(this,root,stage,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_2);

            // Ana "MusaCAD'ı Kullan" düğmesi
            LockedScreenUi.hotspot(this,stage,35,1228,870,115,v->openMusaCad());

            // Alttaki dört yüzer özellik düğmesi de aynı akışı başlatır
            LockedScreenUi.hotspot(this,stage,58,1350,125,135,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,214,1350,125,135,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,370,1350,125,135,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,526,1350,125,135,v->openMusaCad());
        });
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    @Override public void onBackPressed(){
        finish();
    }

    private void openMusaCad(){
        Intent next=LicenseManager.hasAccess(this)
            ?new Intent(this,MainActivity.class)
            :new Intent(this,LicenseActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }
}
