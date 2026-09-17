package com.musa.cad;

import java.util.*;

/** Computes screen-space text placements for complex linetypes. */
public final class DxfComplexLineText {
    public static final class Placement {
        public final String text;public final double x,y,angleDegrees,textSize;
        Placement(String text,double x,double y,double angleDegrees,double textSize){this.text=text;this.x=x;this.y=y;this.angleDegrees=angleDegrees;this.textSize=textSize;}
    }

    public static List<Placement> placements(DxfLineStyle.Pattern pattern,double x1,double y1,double x2,double y2,double pixelsPerPatternUnit){
        return placements(pattern,x1,y1,x2,y2,pixelsPerPatternUnit,0d);
    }

    public static List<Placement> placements(DxfLineStyle.Pattern pattern,double x1,double y1,double x2,double y2,double pixelsPerPatternUnit,double pathOffsetPixels){
        ArrayList<Placement> out=new ArrayList<>();
        if(pattern==null||!pattern.hasRenderableComplexText()||!Double.isFinite(pixelsPerPatternUnit)||pixelsPerPatternUnit<=0d)return out;
        double dx=x2-x1,dy=y2-y1,length=Math.hypot(dx,dy);if(!Double.isFinite(length)||length<1e-6)return out;
        double offset=Double.isFinite(pathOffsetPixels)&&pathOffsetPixels>0d?pathOffsetPixels:0d;
        double cycle=pattern.cycleLength()*pixelsPerPatternUnit;if(!Double.isFinite(cycle)||cycle<1e-4)return out;
        double ux=dx/length,uy=dy/length,lineAngle=Math.toDegrees(Math.atan2(dy,dx));
        for(DxfLineStyle.ComplexElement item:pattern.complexElements){
            if(!item.hasText())continue;
            double first=pattern.elementCenterDistance(item.elementIndex)*pixelsPerPatternUnit;
            double k=Math.ceil((offset-first)/cycle);if(k<0d)k=0d;
            double global=first+k*cycle,local=global-offset;
            double ox=item.xOffset*pixelsPerPatternUnit,oy=item.yOffset*pixelsPerPatternUnit;
            double angle=(item.flags&1)!=0?item.rotationDegrees:lineAngle+item.rotationDegrees;
            double size=Math.max(1d,item.scale*pixelsPerPatternUnit);
            for(double d=local;d<=length+1e-6;d+=cycle){
                if(d<-1e-6)continue;
                double x=x1+ux*d+ux*ox-uy*oy,y=y1+uy*d+uy*ox+ux*oy;
                out.add(new Placement(item.text,x,y,angle,size));
                if(out.size()>=2048)return out;
            }
        }
        return out;
    }
    private DxfComplexLineText(){}
}
