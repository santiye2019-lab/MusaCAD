package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.util.AttributeSet;
import android.view.View;

/** Lightweight blueprint-style background used only by the onboarding screens. */
public class IntroTechnicalBackdropView extends View {
    private final Paint grid=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint cyan=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint blue=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint orange=new Paint(Paint.ANTI_ALIAS_FLAG);

    public IntroTechnicalBackdropView(Context c){super(c);init();}
    public IntroTechnicalBackdropView(Context c,AttributeSet a){super(c,a);init();}
    public IntroTechnicalBackdropView(Context c,AttributeSet a,int s){super(c,a,s);init();}

    private void init(){
        setLayerType(LAYER_TYPE_SOFTWARE,null);
        grid.setStyle(Paint.Style.STROKE);grid.setStrokeWidth(dp(.7f));grid.setColor(Color.argb(30,92,201,220));
        cyan.setStyle(Paint.Style.STROKE);cyan.setStrokeWidth(dp(1.1f));cyan.setColor(Color.argb(85,77,232,218));
        blue.setStyle(Paint.Style.STROKE);blue.setStrokeWidth(dp(1f));blue.setColor(Color.argb(72,67,156,255));
        orange.setStyle(Paint.Style.STROKE);orange.setStrokeWidth(dp(1f));orange.setColor(Color.argb(58,255,170,64));
    }
    private float dp(float v){return v*getResources().getDisplayMetrics().density;}

    @Override protected void onDraw(Canvas c){
        super.onDraw(c);float w=getWidth(),h=getHeight();
        c.drawColor(Color.rgb(5,20,31));
        LinearGradient shade=new LinearGradient(0,0,w,h,Color.rgb(8,47,68),Color.rgb(6,24,36),Shader.TileMode.CLAMP);
        Paint fill=new Paint();fill.setShader(shade);c.drawRect(0,0,w,h,fill);

        float step=dp(34);
        for(float x=0;x<w;x+=step)c.drawLine(x,0,x,h,grid);
        for(float y=0;y<h;y+=step)c.drawLine(0,y,w,y,grid);

        // Building-plan motif.
        RectF plan=new RectF(w*.08f,h*.10f,w*.68f,h*.46f);c.drawRect(plan,blue);
        c.drawRect(w*.13f,h*.15f,w*.38f,h*.30f,cyan);
        c.drawRect(w*.42f,h*.15f,w*.61f,h*.25f,cyan);
        c.drawLine(w*.13f,h*.34f,w*.61f,h*.34f,cyan);
        c.drawLine(w*.26f,h*.15f,w*.26f,h*.42f,cyan);
        c.drawLine(w*.50f,h*.25f,w*.50f,h*.42f,cyan);
        c.drawCircle(w*.33f,h*.38f,dp(8),orange);

        // Mechanical-part / gear motif.
        float cx=w*.79f,cy=h*.25f,r=dp(54);c.drawCircle(cx,cy,r,cyan);c.drawCircle(cx,cy,r*.48f,blue);
        for(int i=0;i<12;i++){double a=i*Math.PI/6;float x1=cx+(float)Math.cos(a)*r*.82f,y1=cy+(float)Math.sin(a)*r*.82f;float x2=cx+(float)Math.cos(a)*r*1.15f,y2=cy+(float)Math.sin(a)*r*1.15f;c.drawLine(x1,y1,x2,y2,orange);}

        // Dimension and piping cues.
        c.drawLine(w*.08f,h*.53f,w*.92f,h*.53f,grid);
        c.drawLine(w*.15f,h*.60f,w*.67f,h*.60f,cyan);
        c.drawLine(w*.67f,h*.60f,w*.67f,h*.75f,cyan);
        c.drawLine(w*.67f,h*.75f,w*.88f,h*.75f,cyan);
        c.drawCircle(w*.67f,h*.60f,dp(5),orange);
        c.drawCircle(w*.67f,h*.75f,dp(5),orange);

        // Fade behind the content cards.
        Paint veil=new Paint();veil.setColor(Color.argb(52,0,0,0));c.drawRect(0,0,w,h,veil);
    }
}
