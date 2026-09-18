package com.musa.cad;

import android.content.*;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Base64;
import android.view.View;
import android.widget.ImageView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.*;

public class OnboardingActivity extends AppCompatActivity {
    private static final String PREFS="musacad_onboarding";
    private static final String K_SEEN="intro_seen_v1";
    private View introPage,bioPage;
    private Intent pendingIntent;

    public static boolean shouldShow(Context context){
        return !context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getBoolean(K_SEEN,false);
    }

    @Override protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        setContentView(R.layout.activity_onboarding);

        View root=findViewById(R.id.onboardingRoot);
        ViewCompat.setOnApplyWindowInsetsListener(root,(v,insets)->{
            Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(0,bars.top,0,bars.bottom);return insets;
        });

        if(android.os.Build.VERSION.SDK_INT>=33)pendingIntent=getIntent().getParcelableExtra(LicenseActivity.EXTRA_PENDING_INTENT,Intent.class);
        else{
            @SuppressWarnings("deprecation") Intent legacy=getIntent().getParcelableExtra(LicenseActivity.EXTRA_PENDING_INTENT);pendingIntent=legacy;
        }

        introPage=findViewById(R.id.introPage);bioPage=findViewById(R.id.bioPage);
        ImageView photo=findViewById(R.id.developerPhoto);Bitmap portrait=decodePortrait();if(portrait!=null)photo.setImageBitmap(portrait);

        findViewById(R.id.introNextButton).setOnClickListener(v->showBio());
        findViewById(R.id.bioBackButton).setOnClickListener(v->showIntro());
        findViewById(R.id.bioContinueButton).setOnClickListener(v->finishIntro());

        introPage.setAlpha(0f);introPage.setTranslationX(28f);
        introPage.animate().alpha(1f).translationX(0f).setDuration(360).start();
    }

    private Bitmap decodePortrait(){
        try(InputStream in=getResources().openRawResource(R.raw.developer_photo_b64);ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buf=new byte[4096];int n;while((n=in.read(buf))!=-1)out.write(buf,0,n);
            byte[] jpg=Base64.decode(out.toString("US-ASCII"),Base64.DEFAULT);
            return BitmapFactory.decodeByteArray(jpg,0,jpg.length);
        }catch(Exception ignored){return null;}
    }

    private void showBio(){
        introPage.animate().alpha(0f).translationX(-36f).setDuration(180).withEndAction(()->{
            introPage.setVisibility(View.GONE);bioPage.setVisibility(View.VISIBLE);bioPage.setAlpha(0f);bioPage.setTranslationX(36f);
            bioPage.animate().alpha(1f).translationX(0f).setDuration(260).start();
        }).start();
    }

    private void showIntro(){
        bioPage.animate().alpha(0f).translationX(36f).setDuration(180).withEndAction(()->{
            bioPage.setVisibility(View.GONE);introPage.setVisibility(View.VISIBLE);introPage.setAlpha(0f);introPage.setTranslationX(-36f);
            introPage.animate().alpha(1f).translationX(0f).setDuration(260).start();
        }).start();
    }

    private void finishIntro(){
        getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().putBoolean(K_SEEN,true).apply();
        Intent next;
        if(LicenseManager.hasAccess(this)){
            next=pendingIntent==null?new Intent(this,MainActivity.class):new Intent(pendingIntent).setClass(this,MainActivity.class);
        }else{
            next=new Intent(this,LicenseActivity.class);
            if(pendingIntent!=null)next.putExtra(LicenseActivity.EXTRA_PENDING_INTENT,pendingIntent);
        }
        startActivity(next);overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);finish();
    }

    @Override public void onBackPressed(){
        if(bioPage!=null&&bioPage.getVisibility()==View.VISIBLE){showIntro();return;}
        super.onBackPressed();
    }
}