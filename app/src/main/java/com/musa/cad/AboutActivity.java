package com.musa.cad;

import android.content.Intent;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
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
            art.setContentDescription("MusaCAD ikinci ekran");

            // Native vector animation: no GIF decoder, no pixelated overlay.
            // It stays only over the 3D preview area and continuously rotates in perspective.
            Cad3dPreviewView preview=new Cad3dPreviewView(this);
            preview.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            stage.addView(preview);
            LockedScreenUi.position(preview,stage,500,350,385,560);

            LockedScreenUi.hotspot(this,stage,35,1248,870,115,v->openMusaCad());
        });
    }

    @Override public void onWindowFocusChanged(boolean hasFocus){
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus)LockedScreenUi.enableImmersive(this);
    }

    @Override public void onBackPressed(){ finish(); }

    private void openMusaCad(){
        Intent next=new Intent(this,LicenseActivity.class);
        next.putExtra(LicenseActivity.EXTRA_STAY_ON_LICENSE,true);
        next.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(next);
        overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
        finish();
    }

    private static final class Cad3dPreviewView extends View {
        private final Paint glow=new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint line=new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint accent=new Paint(Paint.ANTI_ALIAS_FLAG);
        private long start;

        // 3D wireframe points: central valve body, branches, flanges and base.
        private static final float[][] SEGMENTS={
            {-0.42f,-0.15f,-0.42f, 0.42f,-0.15f,-0.42f},{0.42f,-0.15f,-0.42f,0.42f,0.15f,-0.42f},
            {0.42f,0.15f,-0.42f,-0.42f,0.15f,-0.42f},{-0.42f,0.15f,-0.42f,-0.42f,-0.15f,-0.42f},
            {-0.42f,-0.15f,0.42f, 0.42f,-0.15f,0.42f},{0.42f,-0.15f,0.42f,0.42f,0.15f,0.42f},
            {0.42f,0.15f,0.42f,-0.42f,0.15f,0.42f},{-0.42f,0.15f,0.42f,-0.42f,-0.15f,0.42f},
            {-0.42f,-0.15f,-0.42f,-0.42f,-0.15f,0.42f},{0.42f,-0.15f,-0.42f,0.42f,-0.15f,0.42f},
            {0.42f,0.15f,-0.42f,0.42f,0.15f,0.42f},{-0.42f,0.15f,-0.42f,-0.42f,0.15f,0.42f},
            {0f,-0.15f,0f,0f,-0.72f,0f},{0f,-0.72f,0f,0f,-1.02f,0f},
            {-0.25f,-0.72f,0f,0.25f,-0.72f,0f},{-0.30f,-1.02f,0f,0.30f,-1.02f,0f},
            {-0.42f,0f,0f,-0.95f,0.38f,0f},{0.42f,0f,0f,0.95f,0.38f,0f},
            {-0.95f,0.38f,0f,-1.15f,0.38f,0f},{0.95f,0.38f,0f,1.15f,0.38f,0f},
            {-1.15f,0.18f,0f,-1.15f,0.58f,0f},{1.15f,0.18f,0f,1.15f,0.58f,0f},
            {-0.55f,0.15f,-0.55f,-0.55f,0.15f,0.55f},{0.55f,0.15f,-0.55f,0.55f,0.15f,0.55f}
        };

        Cad3dPreviewView(android.content.Context c){
            super(c);
            setLayerType(View.LAYER_TYPE_SOFTWARE,null);
            glow.setStyle(Paint.Style.STROKE);glow.setStrokeWidth(7f);glow.setColor(0x8825B9FF);
            glow.setMaskFilter(new BlurMaskFilter(10f,BlurMaskFilter.Blur.NORMAL));
            line.setStyle(Paint.Style.STROKE);line.setStrokeWidth(2.2f);line.setColor(Color.rgb(190,238,255));
            accent.setStyle(Paint.Style.STROKE);accent.setStrokeWidth(2.7f);accent.setColor(Color.rgb(30,188,255));
        }

        @Override protected void onDraw(Canvas c){
            super.onDraw(c);
            if(start==0L)start=SystemClock.uptimeMillis();
            float t=((SystemClock.uptimeMillis()-start)%8000L)/8000f;
            float angle=(float)(t*Math.PI*2.0);
            float cx=getWidth()*0.50f,cy=getHeight()*0.55f;
            float scale=Math.min(getWidth(),getHeight())*0.28f;

            // Rotating holographic base rings.
            for(int i=0;i<3;i++){
                float r=scale*(1.12f+i*0.12f);
                float phase=angle+i*0.7f;
                float squash=0.24f+0.04f*(float)Math.sin(phase);
                c.drawOval(cx-r,cy+scale*0.88f-r*squash,cx+r,cy+scale*0.88f+r*squash,glow);
                c.drawOval(cx-r,cy+scale*0.88f-r*squash,cx+r,cy+scale*0.88f+r*squash,accent);
            }

            for(float[] s:SEGMENTS){
                float[] a=project(s[0],s[1],s[2],angle,cx,cy,scale);
                float[] b=project(s[3],s[4],s[5],angle,cx,cy,scale);
                c.drawLine(a[0],a[1],b[0],b[1],glow);
                c.drawLine(a[0],a[1],b[0],b[1],line);
            }

            // Central flange circles simulated as rotating ellipses.
            for(int k=0;k<4;k++){
                float y=cy+scale*(-0.14f+k*0.15f);
                float w=scale*(0.46f-k*0.035f);
                float h=scale*0.10f;
                c.drawOval(cx-w,y-h,cx+w,y+h,glow);
                c.drawOval(cx-w,y-h,cx+w,y+h,accent);
            }

            postInvalidateOnAnimation();
        }

        private static float[] project(float x,float y,float z,float a,float cx,float cy,float s){
            float ca=(float)Math.cos(a),sa=(float)Math.sin(a);
            float rx=x*ca+z*sa;
            float rz=-x*sa+z*ca;
            float perspective=1.0f/(1.55f+0.28f*rz);
            return new float[]{cx+rx*s*perspective,cy+y*s*perspective};
        }
    }
}
