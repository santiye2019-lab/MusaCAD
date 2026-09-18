package com.musa.cad;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;

public class SplashActivity extends AppCompatActivity {
    private Intent pendingIntent;
    private static final String STATE_SPLASH_SHOWN="musacad_splash_shown";

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        setContentView(R.layout.activity_splash);
        if(getApplication() instanceof MusaCadApp)((MusaCadApp)getApplication()).markIntroSeen();

        Intent incoming=new Intent(getIntent());
        boolean fileEntry=Intent.ACTION_VIEW.equals(incoming.getAction())||Intent.ACTION_SEND.equals(incoming.getAction());
        pendingIntent=fileEntry?incoming:null;

        View root=findViewById(R.id.splashRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom);
            return insets;
        });

        ((TextView)findViewById(R.id.splashVersion)).setText("v"+BuildConfig.VERSION_NAME);
        View content=findViewById(R.id.splashContent);
        content.setAlpha(0f);
        content.setTranslationY(24f);
        content.animate().alpha(1f).translationY(0f).setDuration(420).start();

        getSharedPreferences("musacad_intro",MODE_PRIVATE).edit().putBoolean(STATE_SPLASH_SHOWN,true).apply();
        findViewById(R.id.startButton).setOnClickListener(v->openApp());
        findViewById(R.id.licenseInfoButton).setOnClickListener(v->openLicense());
        findViewById(R.id.aboutLinkButton).setOnClickListener(v->startActivity(new Intent(this,AboutActivity.class)));
    }

    private void openApp(){
        if(LicenseManager.hasAccess(this)){
            Intent next=pendingIntent==null?new Intent(this,MainActivity.class):new Intent(pendingIntent).setClass(this,MainActivity.class);
            next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            startActivity(next);
        }else{
            Intent next=new Intent(this,LicenseActivity.class);
            if(pendingIntent!=null)next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,pendingIntent);
            startActivity(next);
        }
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
    }

    private void openLicense(){
        Intent next=new Intent(this,LicenseActivity.class);
        if(pendingIntent!=null)next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,pendingIntent);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
    }
}
