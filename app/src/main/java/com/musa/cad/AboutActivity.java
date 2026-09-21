package com.musa.cad;

import android.content.Intent;
import android.os.Bundle;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;

public class AboutActivity extends AppCompatActivity {
    public static final String EXTRA_CONTINUE_TO_APP="com.musa.cad.CONTINUE_TO_APP";
    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_about);
        FrameLayout root=findViewById(R.id.aboutRoot),stage=findViewById(R.id.artworkStage);
        LockedScreenUi.fitStage(this,root,stage,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_2);
            LockedScreenUi.hotspot(this,stage,38,1235,865,100,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,60,1353,125,125,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,214,1353,125,125,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,368,1353,125,125,v->openMusaCad());
            LockedScreenUi.hotspot(this,stage,522,1353,125,125,v->openMusaCad());
        });
    }
    @Override public void onBackPressed(){finish();}
    private void openMusaCad(){
        Intent next=LicenseManager.hasAccess(this)?new Intent(this,MainActivity.class):new Intent(this,LicenseActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        if(getIntent().getBooleanExtra(EXTRA_CONTINUE_TO_APP,false))finish();
    }
}
