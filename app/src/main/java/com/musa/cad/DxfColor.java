package com.musa.cad;

import java.util.Locale;
import java.util.Map;

/** AutoCAD Color Index (ACI), TrueColor, BYLAYER and BYBLOCK resolution. */
public final class DxfColor {
    public static final int BYBLOCK=0;
    public static final int BYLAYER=256;
    public static final int NO_TRUE_COLOR=-1;

    public static final class Ref {
        public final int aci,trueColor;
        public final String layer;
        Ref(int aci,int trueColor,String layer){
            this.aci=aci;this.trueColor=trueColor;this.layer=layer==null?"0":layer;
        }
    }

    /** Resolve a raw entity color. BYBLOCK inherits the parent INSERT/DIMENSION color. */
    public static Ref resolve(int aci,int trueColor,String layer,Ref byBlock){
        if(trueColor>=0)return new Ref(BYLAYER,trueColor&0x00ffffff,layer);
        int value=aci;
        if(value==BYBLOCK){
            if(byBlock!=null)return byBlock;
            return new Ref(BYLAYER,NO_TRUE_COLOR,layer);
        }
        if(value<0)value=Math.abs(value);
        if(value<1||value>255){
            if(value!=BYLAYER)value=BYLAYER;
        }
        return new Ref(value,NO_TRUE_COLOR,layer);
    }

    /** Resolve a semantic color reference to an Android ARGB value. */
    public static int argb(Ref ref,Map<String,Integer> layerColors){
        if(ref==null)return aciArgb(7);
        if(ref.trueColor>=0)return 0xff000000|(ref.trueColor&0x00ffffff);
        if(ref.aci==BYLAYER){
            Integer color=layerColors==null?null:layerColors.get(key(ref.layer));
            return color!=null?color:aciArgb(7);
        }
        return aciArgb(ref.aci);
    }

    /** Layer table colors: TrueColor wins, otherwise ACI. Negative ACI means off but keeps the same color. */
    public static int layerArgb(int aci,int trueColor){
        if(trueColor>=0)return 0xff000000|(trueColor&0x00ffffff);
        int value=Math.abs(aci);
        if(value<1||value>255)value=7;
        return aciArgb(value);
    }

    /** Standard AutoCAD ACI palette, adapted for a dark model-space background (ACI 7 = white). */
    public static int aciArgb(int index){
        int i=Math.abs(index);
        switch(i){
            case 1:return 0xffff0000;
            case 2:return 0xffffff00;
            case 3:return 0xff00ff00;
            case 4:return 0xff00ffff;
            case 5:return 0xff0000ff;
            case 6:return 0xffff00ff;
            case 7:return 0xffffffff;
            case 8:return 0xff808080;
            case 9:return 0xffc0c0c0;
            default:break;
        }
        if(i>=10&&i<=249){
            int group=(i-10)/10;
            int shade=(i-10)%10;
            float hue=group*15f;
            float saturation=(shade%2==0)?1f:.5f;
            float[] values={1f,1f,.65f,.65f,.5f,.5f,.30f,.30f,.15f,.15f};
            return hsv(hue,saturation,values[shade]);
        }
        switch(i){
            case 250:return 0xff333333;
            case 251:return 0xff505050;
            case 252:return 0xff696969;
            case 253:return 0xff828282;
            case 254:return 0xffbebebe;
            case 255:return 0xffffffff;
            default:return 0xffffffff;
        }
    }

    private static int hsv(float h,float s,float v){
        float hh=((h%360f)+360f)%360f/60f;
        int sector=(int)Math.floor(hh);float f=hh-sector;
        float p=v*(1f-s),q=v*(1f-s*f),t=v*(1f-s*(1f-f));
        float r=0,g=0,b=0;
        switch(sector){
            case 0:r=v;g=t;b=p;break;
            case 1:r=q;g=v;b=p;break;
            case 2:r=p;g=v;b=t;break;
            case 3:r=p;g=q;b=v;break;
            case 4:r=t;g=p;b=v;break;
            default:r=v;g=p;b=q;break;
        }
        return 0xff000000|(Math.round(r*255f)<<16)|(Math.round(g*255f)<<8)|Math.round(b*255f);
    }

    public static String key(String layer){return (layer==null?"0":layer).toUpperCase(Locale.ROOT);}
    private DxfColor(){}
}
