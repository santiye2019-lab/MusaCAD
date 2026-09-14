package com.musa.cad;

import java.util.Map;

/** DXF group-code 440 transparency resolution, including BYLAYER/BYBLOCK inheritance. */
public final class DxfTransparency {
    public static final long UNSET=-1L;
    public static final long BYBLOCK_RAW=0x01000000L;
    public static final long EXPLICIT_MASK=0x02000000L;

    public static final class Ref {
        static final int BYLAYER=0,BYBLOCK=1,EXPLICIT=2;
        final int mode,opacity;
        final String layer;
        Ref(int mode,int opacity,String layer){this.mode=mode;this.opacity=opacity;this.layer=layer==null?"0":layer;}
    }

    /** Resolve an entity transparency. Unset/0 means BYLAYER, 0x01000000 means BYBLOCK. */
    public static Ref resolve(long raw,String layer,Ref byBlock){
        long value=raw&0xffffffffL;
        if(raw<0||raw==0)return new Ref(Ref.BYLAYER,255,layer);
        long method=value&0xff000000L;
        if(method==BYBLOCK_RAW){
            if(byBlock!=null)return byBlock;
            return new Ref(Ref.BYLAYER,255,layer);
        }
        if(method==EXPLICIT_MASK)return new Ref(Ref.EXPLICIT,(int)(value&0xffL),layer);
        // Be permissive with converters that emit only the low alpha byte.
        if(value<=255L)return new Ref(Ref.EXPLICIT,(int)value,layer);
        return new Ref(Ref.BYLAYER,255,layer);
    }

    /** Layer transparency defaults to fully opaque when no explicit alpha is present. */
    public static int layerOpacity(long raw){
        if(raw<0||raw==0)return 255;
        long value=raw&0xffffffffL;
        if((value&0xff000000L)==EXPLICIT_MASK)return clamp((int)(value&0xffL));
        if(value<=255L)return clamp((int)value);
        return 255;
    }

    /** Resolve semantic opacity after BYLAYER inheritance. */
    public static int opacity(Ref ref,Map<String,Integer> layerOpacities){
        if(ref==null)return 255;
        if(ref.mode==Ref.EXPLICIT)return clamp(ref.opacity);
        if(ref.mode==Ref.BYLAYER&&layerOpacities!=null){
            Integer alpha=layerOpacities.get(DxfColor.key(ref.layer));
            if(alpha!=null)return clamp(alpha);
        }
        return 255;
    }

    /** Replace an ARGB color's alpha channel while preserving RGB. */
    public static int apply(int argb,int opacity){return (clamp(opacity)<<24)|(argb&0x00ffffff);}

    private static int clamp(int value){return Math.max(0,Math.min(255,value));}
    private DxfTransparency(){}
}
