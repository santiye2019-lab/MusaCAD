package com.musa.cad;

import android.content.Context;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Movie;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.appcompat.app.AppCompatActivity;
import java.io.InputStream;

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
            art.setContentDescription("MusaCAD ikinci ekran");

            // Yüksek çözünürlüklü ikinci ekranın sağındaki 3D bölüm animasyonlu tutulur.
            GifMovieView overlay=new GifMovieView(this);
            overlay.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            stage.addView(overlay);
            LockedScreenUi.position(overlay,stage,390,260,540,820);
            overlay.setGifResource(R.raw.musacad_screen2_3d_overlay);

            // Görseldeki ana "MusaCAD'ı Kullan" düğmesi aktiftir.
            LockedScreenUi.hotspot(this,stage,35,1248,870,115,v->openMusaCad());
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
        // Ekran akışı: 1) Splash -> 2) Tanıtım/3D -> 3) Lisans.
        // Geçerli lisans/deneme varsa LicenseActivity kendi kontrolüyle ana uygulamaya devam eder.
        Intent next=new Intent(this,LicenseActivity.class);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }

    private static final class GifMovieView extends View {
        private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        private Movie movie;
        private long animationStart;

        GifMovieView(Context context){
            super(context);
            setLayerType(View.LAYER_TYPE_SOFTWARE,null);
            setClickable(false);
            setFocusable(false);
        }

        void setGifResource(int resId){
            try(InputStream in=getResources().openRawResource(resId)){
                movie=Movie.decodeStream(in);
                animationStart=0L;
                invalidate();
            }catch(Exception ignored){
                movie=null;
            }
        }

        @Override protected void onDraw(Canvas canvas){
            super.onDraw(canvas);
            if(movie==null||movie.width()<=0||movie.height()<=0)return;

            int duration=movie.duration();
            if(duration<=0)duration=1000;

            long now=SystemClock.uptimeMillis();
            if(animationStart==0L)animationStart=now;
            movie.setTime((int)((now-animationStart)%duration));

            float sx=getWidth()/(float)movie.width();
            float sy=getHeight()/(float)movie.height();

            canvas.save();
            canvas.scale(sx,sy);
            movie.draw(canvas,0f,0f,paint);
            canvas.restore();
            postInvalidateOnAnimation();
        }
    }
}
