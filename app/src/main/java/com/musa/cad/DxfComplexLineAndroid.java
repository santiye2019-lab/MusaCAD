package com.musa.cad;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import java.util.List;

/** Android drawing adapter for complex-linetype embedded text. */
final class DxfComplexLineAndroid {
    static void drawSegment(Canvas canvas,Paint paint,DxfLineStyle.Pattern pattern,float x1,float y1,float x2,float y2,double pixelsPerPatternUnit){
        drawSegment(canvas,paint,pattern,x1,y1,x2,y2,pixelsPerPatternUnit,0d,1d,0d);
    }
    static void drawSegment(Canvas canvas,Paint paint,DxfLineStyle.Pattern pattern,float x1,float y1,float x2,float y2,double pixelsPerPatternUnit,double pathOffsetPixels){
        drawSegment(canvas,paint,pattern,x1,y1,x2,y2,pixelsPerPatternUnit,pathOffsetPixels,1d,0d);
    }
    static void drawSegment(Canvas canvas,Paint paint,DxfLineStyle.Pattern pattern,float x1,float y1,float x2,float y2,double pixelsPerPatternUnit,double pathOffsetPixels,double orientationSign,double absoluteXAxisAngleDegrees){
        if(canvas==null||paint==null||pattern==null||!pattern.hasRenderableComplexText())return;
        List<DxfComplexLineText.Placement> placements=DxfComplexLineText.placements(pattern,x1,y1,x2,y2,pixelsPerPatternUnit,pathOffsetPixels,orientationSign,absoluteXAxisAngleDegrees);
        if(placements.isEmpty())return;
        Paint.Style oldStyle=paint.getStyle();Paint.Align oldAlign=paint.getTextAlign();Typeface oldTypeface=paint.getTypeface();
        float oldSize=paint.getTextSize();android.graphics.PathEffect oldEffect=paint.getPathEffect();
        paint.setPathEffect(null);paint.setStyle(Paint.Style.FILL);paint.setTextAlign(Paint.Align.CENTER);paint.setTypeface(Typeface.MONOSPACE);
        for(DxfComplexLineText.Placement item:placements){
            if(item.text==null||item.text.isEmpty())continue;
            paint.setTextSize((float)Math.max(5d,Math.min(72d,item.textSize)));
            int save=canvas.save();canvas.rotate((float)item.angleDegrees,(float)item.x,(float)item.y);
            canvas.drawText(item.text,(float)item.x,(float)item.y,paint);canvas.restoreToCount(save);
        }
        paint.setPathEffect(oldEffect);paint.setTextSize(oldSize);paint.setTypeface(oldTypeface);paint.setTextAlign(oldAlign);paint.setStyle(oldStyle);
    }
    private DxfComplexLineAndroid(){}
}
