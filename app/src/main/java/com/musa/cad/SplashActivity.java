package com.musa.cad;

import android.content.*;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Base64;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.ViewFlipper;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.*;

public class SplashActivity extends AppCompatActivity {
    private static final String PREFS="musacad_intro";
    private static final String KEY_DONE="professional_intro_done";
    private ViewFlipper flipper;
    private int page;

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(getSharedPreferences(PREFS,MODE_PRIVATE).getBoolean(KEY_DONE,false)){
            routeNext();return;
        }

        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_splash);
        View root=findViewById(R.id.splashRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom);return insets;
        });

        flipper=findViewById(R.id.introFlipper);
        loadDeveloperPhoto();

        findViewById(R.id.introNextButton).setOnClickListener(v->showPage(1));
        findViewById(R.id.introBackButton).setOnClickListener(v->showPage(0));
        findViewById(R.id.introContinueButton).setOnClickListener(v->{
            getSharedPreferences(PREFS,MODE_PRIVATE).edit().putBoolean(KEY_DONE,true).apply();
            routeNext();
        });
    }

    private void showPage(int target){
        if(flipper==null||target==page)return;
        boolean forward=target>page;
        flipper.setInAnimation(AnimationUtils.loadAnimation(this,forward?android.R.anim.slide_in_left:android.R.anim.fade_in));
        flipper.setOutAnimation(AnimationUtils.loadAnimation(this,android.R.anim.fade_out));
        flipper.setDisplayedChild(target);page=target;
    }

    private void loadDeveloperPhoto(){
        ImageView photo=findViewById(R.id.developerPhoto);
        if(photo==null)return;
        try(InputStream in=getResources().openRawResource(R.raw.developer_photo_b64);
            BufferedReader reader=new BufferedReader(new InputStreamReader(in))){
            StringBuilder text=new StringBuilder();String line;
            while((line=reader.readLine())!=null)text.append(line.trim());
            byte[] bytes=Base64.decode(text.toString(),Base64.DEFAULT);
            Bitmap bitmap=BitmapFactory.decodeByteArray(bytes,0,bytes.length);
            if(bitmap!=null)photo.setImageBitmap(bitmap);
        }catch(Exception ignored){
            photo.setImageResource(R.drawable.ic_musacad_mark);
        }
    }

    private void routeNext(){
        if(isFinishing()||isDestroyed())return;
        Intent incoming=new Intent(getIntent());
        boolean fileEntry=Intent.ACTION_VIEW.equals(incoming.getAction())||Intent.ACTION_SEND.equals(incoming.getAction());
        Intent next;
        if(LicenseManager.hasAccess(this)){
            next=fileEntry?incoming.setClass(this,MainActivity.class):new Intent(this,MainActivity.class);
        }else{
            next=new Intent(this,LicenseActivity.class);
            if(fileEntry)next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,incoming);
        }
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }

    @Override public void onBackPressed(){
        if(flipper!=null&&page==1){showPage(0);return;}
        super.onBackPressed();
    }
}
