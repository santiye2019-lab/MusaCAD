package com.musa.cad;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import java.io.InputStream;

final class LockedScreenUi {
    static final float ART_W=941f, ART_H=1672f;
    private LockedScreenUi(){}

    static void fitStage(Activity a, FrameLayout root, FrameLayout stage, Runnable ready){
        root.post(()->{
            int rw=root.getWidth(), rh=root.getHeight();
            if(rw<=0||rh<=0)return;
            float scale=Math.min(rw/ART_W,rh/ART_H);
            int w=Math.max(1,Math.round(ART_W*scale));
            int h=Math.max(1,Math.round(ART_H*scale));
            FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(w,h,Gravity.CENTER);
            stage.setLayoutParams(lp);
            stage.removeAllViews();

            ImageView art=new ImageView(a);
            art.setId(R.id.lockedArtwork);
            art.setScaleType(ImageView.ScaleType.FIT_XY);
            art.setAdjustViewBounds(false);
            stage.addView(art,new FrameLayout.LayoutParams(-1,-1));
            if(ready!=null)ready.run();
        });
    }

    static void loadArtwork(Activity a, ImageView target, String assetPrefix, int fallbackDrawable){
        try(InputStream in=a.getAssets().open(assetPrefix+".webp")){
            Bitmap bmp=BitmapFactory.decodeStream(in);
            if(bmp!=null){
                target.setImageBitmap(bmp);
                target.setAlpha(1f);
                target.setVisibility(View.VISIBLE);
                return;
            }
        }catch(Throwable ignored){}
        target.setImageResource(fallbackDrawable);
        target.setAlpha(1f);
        target.setVisibility(View.VISIBLE);
    }

    static View hotspot(Activity a, FrameLayout stage, float x,float y,float w,float h, View.OnClickListener click){
        View v=new View(a);
        v.setClickable(true);
        v.setFocusable(true);
        v.setContentDescription("MusaCAD işlem alanı");
        v.setBackground(new ColorDrawable(0x01000000));
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(
            Math.max(1,Math.round(stage.getWidth()*w/ART_W)),
            Math.max(1,Math.round(stage.getHeight()*h/ART_H))
        );
        lp.leftMargin=Math.round(stage.getWidth()*x/ART_W);
        lp.topMargin=Math.round(stage.getHeight()*y/ART_H);
        stage.addView(v,lp);
        v.setOnClickListener(click);
        v.setOnTouchListener((view,e)->{
            if(e.getAction()==android.view.MotionEvent.ACTION_DOWN)view.setAlpha(.72f);
            else if(e.getAction()==android.view.MotionEvent.ACTION_UP||e.getAction()==android.view.MotionEvent.ACTION_CANCEL)view.setAlpha(1f);
            return false;
        });
        return v;
    }

    static void position(View v, FrameLayout stage, float x,float y,float w,float h){
        FrameLayout.LayoutParams lp=new FrameLayout.LayoutParams(
            Math.max(1,Math.round(stage.getWidth()*w/ART_W)),
            Math.max(1,Math.round(stage.getHeight()*h/ART_H))
        );
        lp.leftMargin=Math.round(stage.getWidth()*x/ART_W);
        lp.topMargin=Math.round(stage.getHeight()*y/ART_H);
        v.setLayoutParams(lp);
        v.bringToFront();
    }
}
