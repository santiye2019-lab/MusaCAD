package com.musa.cad;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
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
        View start=findViewById(R.id.startButtonHotspot);

        // Only the visible "Hadi Başlayalım" button continues the onboarding flow.
        start.setOnClickListener(v->continueFlow());
        start.setOnTouchListener((v,e)->{
            if(e.getAction()==MotionEvent.ACTION_DOWN)v.setBackgroundColor(0x1822A7FF);
            else if(e.getAction()==MotionEvent.ACTION_UP||e.getAction()==MotionEvent.ACTION_CANCEL)
                v.setBackgroundColor(Color.TRANSPARENT);
            return false;
        });

        root.post(()->positionStartButton(root,start));
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    private void positionStartButton(FrameLayout root,View start){
        if(root.getWidth()<=0||root.getHeight()<=0)return;

        float scale=Math.min(
            root.getWidth()/LockedScreenUi.ART_W,
            root.getHeight()/LockedScreenUi.ART_H
        );
        float shownW=LockedScreenUi.ART_W*scale;
        float shownH=LockedScreenUi.ART_H*scale;
        float offsetX=(root.getWidth()-shownW)/2f;
        float offsetY=(root.getHeight()-shownH)/2f;

        // Generated artwork coordinates for the visible "Hadi Başlayalım" CTA.
        float x=175f,y=1410f,w=590f,h=135f;
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(
            Math.max(1,Math.round(w*scale)),
            Math.max(1,Math.round(h*scale))
        );
        lp.leftMargin=Math.round(offsetX+x*scale);
        lp.topMargin=Math.round(offsetY+y*scale);
        start.setLayoutParams(lp);
        start.bringToFront();
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
