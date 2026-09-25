package com.musa.cad;

import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class SplashActivity extends AppCompatActivity {
    private static final float SCREEN_W=600f,SCREEN_H=1535f;
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

        LockedScreenUi.fillStage(this,root,stage,SCREEN_W,SCREEN_H,()->{
            ImageView art=stage.findViewById(R.id.lockedArtwork);
            art.setImageResource(R.drawable.musacad_screen_1);
            art.setContentDescription("MusaCAD açılış ekranı");

            // 3D capability is now explicitly visible on the first screen.
            TextView threeD=new TextView(this);
            threeD.setText("2D + 3D CAD DESTEĞİ");
            threeD.setTextColor(Color.WHITE);
            threeD.setTextSize(13f);
            threeD.setTypeface(Typeface.DEFAULT,Typeface.BOLD);
            threeD.setGravity(Gravity.CENTER);
            threeD.setLetterSpacing(.05f);
            threeD.setContentDescription("MusaCAD iki boyutlu ve üç boyutlu CAD desteği");
            GradientDrawable badgeBg=new GradientDrawable();
            badgeBg.setColor(Color.argb(220,5,35,51));
            badgeBg.setCornerRadius(dp(18));
            badgeBg.setStroke(dp(1),Color.rgb(50,205,225));
            threeD.setBackground(badgeBg);
            stage.addView(threeD);
            LockedScreenUi.position(threeD,stage,152,335,295,57,SCREEN_W,SCREEN_H);

            // Deliberate CTA: the intro no longer advances from invisible hotspots.
            Button start=new Button(this);
            start.setText("Hadi Başlayalım");
            start.setAllCaps(false);
            start.setTextColor(Color.WHITE);
            start.setTextSize(17f);
            start.setTypeface(Typeface.DEFAULT,Typeface.BOLD);
            start.setGravity(Gravity.CENTER);
            start.setPadding(dp(12),0,dp(12),0);
            start.setStateListAnimator(null);
            start.setContentDescription("Hadi başlayalım, MusaCAD'e devam et");
            GradientDrawable buttonBg=new GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                new int[]{Color.rgb(0,151,178),Color.rgb(0,201,211)}
            );
            buttonBg.setCornerRadius(dp(24));
            buttonBg.setStroke(dp(1),Color.argb(210,255,255,255));
            start.setBackground(buttonBg);
            start.setOnClickListener(v->{
                v.performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP);
                continueFlow();
            });
            stage.addView(start);
            LockedScreenUi.position(start,stage,109,1197,382,84,SCREEN_W,SCREEN_H);
        });
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    private int dp(int value){
        return Math.round(value*getResources().getDisplayMetrics().density);
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
