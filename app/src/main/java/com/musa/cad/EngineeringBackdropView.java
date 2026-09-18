package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.util.AttributeSet;
import android.view.View;

/** Decorative blueprint-style background: grid, plan lines and mechanical geometry. */
public class EngineeringBackdropView extends View {
    private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path=new Path();
    public EngineeringBackdropView(Context c){super(c);}
    public EngineeringBackdropView(Context c,AttributeSet a){super(c,a);}
    public EngineeringBackdropView(Context c,AttributeSet a,int s){super(c,a,s);}

    @Override protected void onDraw(Canvas c){
        super.onDraw(c);float w=getWidth(),h=getHeight();
        Paint bg=new Paint();bg.setShader(new LinearGradient(0,0,w,h,
            new int[]{Color.rgb(4,20,35),Color.rgb(5,43,70),Color.rgb(3,23,39)},null,Shader.TileMode.CLAMP));
        c.drawRect(0,0,w,h,bg);

        p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(dp(.7f));p.setColor(Color.argb(35,65,190,255));
        float grid=dp(28);for(float x=0;x<w;x+=grid)c.drawLine(x,0,x,h,p);for(float y=0;y<h;y+=grid)c.drawLine(0,y,w,y,p);

        p.setStrokeWidth(dp(1.2f));p.setColor(Color.argb(72,57,185,255));
        drawPlan(c,w*.08f,h*.10f,w*.62f,h*.36f);
        drawBuilding(c,w*.42f,h*.50f,w*.48f,h*.30f);
        drawGear(c,w*.13f,h*.72f,dp(58),16);
        drawGear(c,w*.77f,h*.18f,dp(38),12);
        drawShaft(c,w*.10f,h*.84f,w*.58f,h*.92f);
    }

    private void drawPlan(Canvas c,float x,float y,float ww,float hh){
        c.drawRect(x,y,x+ww,y+hh,p);float a=x+ww*.18f,b=x+ww*.48f,d=x+ww*.78f;
        c.drawLine(a,y,a,y+hh,p);c.drawLine(b,y,b,y+hh,p);c.drawLine(d,y,d,y+hh,p);
        c.drawLine(x,y+hh*.35f,x+ww,y+hh*.35f,p);c.drawLine(x,y+hh*.68f,x+ww,y+hh*.68f,p);
        for(int i=0;i<4;i++){float cx=x+ww*(.12f+i*.22f);c.drawCircle(cx,y+hh*.53f,dp(7),p);}
        p.setStrokeWidth(dp(.7f));for(int i=0;i<5;i++)c.drawRect(x+dp(7+i*4),y+dp(7+i*4),x+ww-dp(7+i*4),y+hh-dp(7+i*4),p);p.setStrokeWidth(dp(1.2f));
    }
    private void drawBuilding(Canvas c,float x,float y,float ww,float hh){
        c.drawRect(x,y,x+ww,y+hh,p);
        for(int i=1;i<5;i++){float yy=y+hh*i/5f;c.drawLine(x,yy,x+ww,yy,p);}
        for(int i=1;i<4;i++){float xx=x+ww*i/4f;c.drawLine(xx,y,xx,y+hh,p);}
        path.reset();path.moveTo(x-dp(14),y);path.lineTo(x+ww*.5f,y-dp(42));path.lineTo(x+ww+dp(14),y);c.drawPath(path,p);
        c.drawLine(x+ww*.08f,y+hh,x+ww*.08f,y+hh+dp(35),p);c.drawLine(x+ww*.92f,y+hh,x+ww*.92f,y+hh+dp(35),p);
    }
    private void drawGear(Canvas c,float cx,float cy,float r,int teeth){
        c.drawCircle(cx,cy,r,p);c.drawCircle(cx,cy,r*.34f,p);
        for(int i=0;i<teeth;i++){double a=Math.PI*2*i/teeth;float x1=(float)(cx+Math.cos(a)*r);float y1=(float)(cy+Math.sin(a)*r);float x2=(float)(cx+Math.cos(a)*(r+dp(12)));float y2=(float)(cy+Math.sin(a)*(r+dp(12)));c.drawLine(x1,y1,x2,y2,p);}
    }
    private void drawShaft(Canvas c,float x1,float y1,float x2,float y2){
        c.drawLine(x1,y1,x2,y2,p);c.drawLine(x1,y1+dp(18),x2,y2+dp(18),p);
        for(int i=0;i<6;i++){float t=i/5f;float x=x1+(x2-x1)*t,y=y1+(y2-y1)*t;c.drawCircle(x,y+dp(9),dp(8+i%2*3),p);}
    }
    private float dp(float v){return v*getResources().getDisplayMetrics().density;}
}