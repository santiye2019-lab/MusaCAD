package com.musa.cad;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.ColorDrawable;
import android.util.Base64;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
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
            stage.addView(art,new FrameLayout.LayoutParams(-1,-1));
            if(ready!=null)ready.run();
        });
    }

    static void loadArtwork(Activity a, ImageView target, String assetPrefix, int fallbackDrawable){
        try{
            StringBuilder encoded=new StringBuilder();
            for(int i=0;i<99;i++){
                String name=assetPrefix+"_"+String.format(java.util.Locale.US,"%02d",i)+".b64";
                try(InputStream in=a.getAssets().open(name)){
                    ByteArrayOutputStream out=new ByteArrayOutputStream();
                    byte[] buf=new byte[8192]; int n;
                    while((n=in.read(buf))!=-1)out.write(buf,0,n);
                    encoded.append(out.toString("UTF-8"));
                }catch(IOException missing){break;}
            }
            if(encoded.length()>0){
                byte[] bytes=Base64.decode(encoded.toString(),Base64.DEFAULT);
                Bitmap bmp=BitmapFactory.decodeByteArray(bytes,0,bytes.length);
                if(bmp!=null){target.setImageBitmap(bmp);return;}
            }
        }catch(Throwable ignored){}
        target.setImageResource(fallbackDrawable);
    }

    static View hotspot(Activity a, FrameLayout stage, float x,float y,float w,float h, View.OnClickListener click){
        View v=new View(a);
        v.setClickable(true); v.setFocusable(true);
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
