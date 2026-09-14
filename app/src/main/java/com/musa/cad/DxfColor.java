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
        if(value<1||value>255){if(value!=BYLAYER)value=BYLAYER;}
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
        int value=Math.abs(aci);if(value<1||value>255)value=7;
        return aciArgb(value);
    }

    /** Standard AutoCAD ACI palette for dark model space (ACI 7 = white). */
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
            int group=(i-10)/10, shade=(i-10)%10;
            float hue=group*15f;
            int[] values={255,255,165,165,127,127,76,76,38,38};
            float saturation=(shade&1)==0?1f:.5f;
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

    private static int hsv(float hue,float saturation,int value){
        float h=((hue%360f)+360f)%360f/60f;
        int sector=(int)Math.floor(h);float f=h-sector;
        float p=value*(1f-saturation),q=value*(1f-saturation*f),t=value*(1f-saturation*(1f-f));
        int r=0,g=0,b=0;
        switch(sector){
            case 0:r=value;g=(int)t;b=(int)p;break;
            case 1:r=(int)q;g=value;b=(int)p;break;
            case 2:r=(int)p;g=value;b=(int)t;break;
            case 3:r=(int)p;g=(int)q;b=value;break;
            case 4:r=(int)t;g=(int)p;b=value;break;
            default:r=value;g=(int)p;b=(int)q;break;
        }
        return 0xff000000|(r<<16)|(g<<8)|b;
    }

    public static String key(String layer){return (layer==null?"0":layer).toUpperCase(Locale.ROOT);}
    private DxfColor(){}
}
