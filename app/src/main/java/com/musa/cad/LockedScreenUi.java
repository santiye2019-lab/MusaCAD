package com.musa.cad;

import android.app.Activity;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

final class LockedScreenUi {
    static final float ART_W=941f, ART_H=1672f;
    private LockedScreenUi(){}

    static void enableImmersive(Activity a){
        WindowCompat.setDecorFitsSystemWindows(a.getWindow(),false);
        WindowInsetsControllerCompat controller=
            WindowCompat.getInsetsController(a.getWindow(),a.getWindow().getDecorView());
        if(controller!=null){
            controller.hide(WindowInsetsCompat.Type.statusBars()|WindowInsetsCompat.Type.navigationBars());
            controller.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            );
        }
        if(Build.VERSION.SDK_INT>=28){
            WindowManager.LayoutParams lp=a.getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode=WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            a.getWindow().setAttributes(lp);
        }
    }

    static void fillStage(Activity a, FrameLayout root, FrameLayout stage, Runnable ready){
        fillStage(a,root,stage,ART_W,ART_H,ready);
    }

    static void fillStage(Activity a, FrameLayout root, FrameLayout stage, float artW,float artH,Runnable ready){
        root.post(()->{
            if(root.getWidth()<=0||root.getHeight()<=0)return;

            // Preserve each locked artwork's native aspect ratio so text and graphics never stretch.
            float scale=Math.min(root.getWidth()/artW,root.getHeight()/artH);
            int stageW=Math.max(1,Math.round(artW*scale));
            int stageH=Math.max(1,Math.round(artH*scale));
            FrameLayout.LayoutParams stageLp=
                new FrameLayout.LayoutParams(stageW,stageH,Gravity.CENTER);
            stage.setLayoutParams(stageLp);
            stage.removeAllViews();

            ImageView art=new ImageView(a);
            art.setId(R.id.lockedArtwork);
            art.setScaleType(ImageView.ScaleType.FIT_XY);
            art.setAdjustViewBounds(false);
            stage.addView(art,new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            ));

            stage.post(()->{
                if(ready!=null)ready.run();
            });
        });
    }

    static View hotspot(Activity a, FrameLayout stage, float x,float y,float w,float h, View.OnClickListener click){
        return hotspot(a,stage,x,y,w,h,ART_W,ART_H,click);
    }

    static View hotspot(Activity a, FrameLayout stage, float x,float y,float w,float h,
                        float artW,float artH,View.OnClickListener click){
        View v=new View(a);
        v.setClickable(true);
        v.setFocusable(true);
        v.setContentDescription("MusaCAD işlem alanı");
        v.setBackground(new ColorDrawable(0x01000000));
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(
            Math.max(1,Math.round(stage.getWidth()*w/artW)),
            Math.max(1,Math.round(stage.getHeight()*h/artH))
        );
        lp.leftMargin=Math.round(stage.getWidth()*x/artW);
        lp.topMargin=Math.round(stage.getHeight()*y/artH);
        stage.addView(v,lp);
        v.bringToFront();
        v.setOnClickListener(click);
        v.setOnTouchListener((view,e)->{
            if(e.getAction()==android.view.MotionEvent.ACTION_DOWN)view.setAlpha(.70f);
            else if(e.getAction()==android.view.MotionEvent.ACTION_UP
                ||e.getAction()==android.view.MotionEvent.ACTION_CANCEL)view.setAlpha(1f);
            return false;
        });
        return v;
    }

    static void position(View v, FrameLayout stage, float x,float y,float w,float h){
        position(v,stage,x,y,w,h,ART_W,ART_H);
    }

    static void position(View v, FrameLayout stage, float x,float y,float w,float h,float artW,float artH){
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(
            Math.max(1,Math.round(stage.getWidth()*w/artW)),
            Math.max(1,Math.round(stage.getHeight()*h/artH))
        );
        lp.leftMargin=Math.round(stage.getWidth()*x/artW);
        lp.topMargin=Math.round(stage.getHeight()*y/artH);
        v.setLayoutParams(lp);
        v.bringToFront();
    }
}
