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

    private static final int[] ACI={
        0xFF000000,
        0xFFFF0000,
        0xFFFFFF00,
        0xFF00FF00,
        0xFF00FFFF,
        0xFF0000FF,
        0xFFFF00FF,
        0xFFFFFFFF,
        0xFF808080,
        0xFFC0C0C0,
        0xFFFF0000,
        0xFFFF7F7F,
        0xFFA50000,
        0xFFA55252,
        0xFF7F0000,
        0xFF7F3F3F,
        0xFF4C0000,
        0xFF4C2626,
        0xFF260000,
        0xFF261313,
        0xFFFF3F00,
        0xFFFF9F7F,
        0xFFA52900,
        0xFFA56752,
        0xFF7F1F00,
        0xFF7F4F3F,
       0xFF4C1300,
        0xFF4C2F26,
        0xFF260900,
        0xFF261713,
        0xFFFF7F000,
        0xFFFFBF7F,
        0xFFA55200,
        0xFFA57C52,
        0xFF7F3F00,
        0xFF7F5F3F,
        0xFF4C2600,
        0xFF4C3926,
        0xFF261300,
        0xFF261C13,
        0xFFFFBF00,
        0xFFFFDF7F,
        0xFFA57C00,
        0xFFA59152,
        0xFF7F5F00,
        0xFF7F6F3F,
        0xFF4C3900,
        0xFF4C4226,
        0xFF261C00,
        0xFF262113,
        0xFFFFFF00,
        0xFFFFFF7F,
        0xFFA5A500,
        0xFFA5A552,
        0xFF7F7F00,
        0xFF7F7F3F,
        0xFF4C4C00,
        0xFF4C4C26,
        0xFF262600,
        0xFF262613,
        0xFFBFFF00,
        0xFFDFFF7F,
        0xFF7CA500,
        0xFF91A552,
        0xFF5F7F00,
        0xFF6F7F3F,
        0xFF394C00,
        0xFF424C26,
        0xFF1C2600,
        0xFF212613,
        0xFF7FFF00,
        0xFFBFFF7F,
        0xFF52A500,
        0xFF7CA552,
        0xFF3F7F00,
        0xFF5F7F3F,
        0xFF264C00,
        0xFF394C26,
        0xFF132600,
        0xFF1C2613,
        0xFF3FFF00,
        0xFF9FFF7F,
        0xFF29A500,
        0xFF67A552,
        0xFF1F7F00,
        0xFF4F7F3F,
        0xFF134C00,
        0xFF2F4C26,
        0xFF092600,
        0xFF172613,
        0xFF00FF00,
        0xFF7FFF7F,
        0xFF00A500,
        0xFF52A552,
        0xFF007F00,
        0xFF3F7F3F,
        0xFF004C00,
        0xFF264C26,
        0xFF002600,
        0xFF132613,
        0xFF00FF3F,
        0xFF7FFF9F,
        0xFF00A529,
        0xFF52A567,
        0xFF007F1F,
        0xFF3F7F4F,
        0xFF004C13,
        0xFF264C2F,
        0xFF002609,
        0xFF135817,
        0xFF00FF7F,
        0xFF7FFFBF,
        0xFF00A552,
        0xFF52A57C,
        0xFF007F3F,
        0xFF3F7F5F,
        0xFF004C26,
        0xFF264C39,
        0xFF002613,
        0xFF13581C,
        0xFF00FFBF,
        0xFF7FFFDF,
        0xFF00A57C,
        0xFF52A591,
        0xFF007F5F,
        0xFF3F7F6F,
        0xFF004C39,
        0xFF264C42,
        0xFF00261C,
        0xFF135858,
        0xFF00FFFF,
        0xFF7FFFFF,
        0xFF00A5A5,
        0xFF52A5A5,
        0xFF007F7F,
        0xFF3F7F7F,
        0xFF004C4C,
        0xFF264C4C,
        0xFF002626,
        0xFF135858,
        0xFF00BFFF,
        0xFF7FDFFF,
        0xFF007CA5,
        0xFF5291A5,
        0xFF005F7F,
        0xFF3F6F7F,
        0xFF00394C,
        0xFF26427E,
        0xFF001C26,
        0xFF135858,
        0xFF007FFF,
        0xFF7FBFFF,
        0xFF0052A5,
        0xFF527CA5,
        0xFF003F7F,
        0xFF3F5F7F,
        0xFF00264C,
        0xFF26397E,
        0xFF001326,
        0xFF131C58,
        0xFF003FFF,
        0xFF7F9FFF,
        0xFF0029A5,
        0xFF5267A5,
        0xFF001F7F,
        0xFF3F4F7F,
        0xFF00134C,
        0xFF262F7E,
        0xFF000926,
        0xFF131758,
        0xFF0000FF,
        0xFF7F7FFF,
        0xFF0000A5,
        0xFF5252A5,
        0xFF00007F,
        0xFF3F3F7F,
        0xFF00004C,
        0xFF26267E,
        0xFF000026,
        0xFF131358,
        0xFF3F00FF,
        0xFF9F7FFF,
        0xFF2900A5,
        0xFF6752A5,
        0xFF1F007F,
        0xFF4F3F7F,
        0xFF13004C,
        0xFF2F267E,
        0xFF090026,
        0xFF171358,
        0xFF7F00FF,
        0xFFBF7FFF,
        0xFF5200A5,
        0xFF7C52A5,
        0xFF3F007F,
        0xFF5F3F7F,
        0xFF26004C,
        0xFF39267E,
        0xFF130026,
        0xFF1C1358,
        0xFFBF00FF,
        0xFFDF7FFF,
        0xFF7C00A5,
        0xFF9152A5,
        0xFF5F007F,
        0xFF6F3F7F,
        0xFF39004C,
        0xFF42264C,
        0xFF1C0026,
        0xFF581358,
        0xFFFF00FF,
        0xFFFF7FFF,
        0xFFA500A5,
        0xFFA552A5,
        0xFF7F007F,
        0xFF7F3F7F,
        0xFF4C004C,
        0xFF4C264C,
        0xFF260026,
        0xFF581358,
        0xFFFF00BF,
        0xFFFF7FDF,
        0xFFA5007C,
        0xFFA55291,
        0xFF7F005F,
        0xFF7F3F6F,
        0xFF4C0039,
        0xFF4C2642,
        0xFF26001C,
        0xFF581358,
        0xFFFF007F,
        0xFFFF7FBF,
        0xFFA50052,
        0xFFA5527C,
        0xFF7F003F,
        0xFF7F3F5F,
        0xFF4C0026,
        0xFF4C2639,
        0xFF260013,
        0xFF58131C,
        0xFFFF003F,
        0xFFFF7F9F,
        0xFFA50029,
        0xFFA55267,
        0xFF7F001F,
        0xFF7F3F4F,
        0xFF4C0013,
        0xFF4C262F,
        0xFF260009,
        0xFF581317,
        0xFF000000,
        0xFF656565,
        0xFF666666,
        0xFF999999,
        0xFFCCCCCC,
        0xFFFFFFFF
    };

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

    /** Exact 256-entry AutoCAD ACI palette. ACI 7 is white for the dark model-space canvas. */
    public static int aciArgb(int index){
        int i=Math.abs(index);
        if(i<1||i>255)i=7;
        return ACI[i];
    }

    public static String key(String layer){return (layer==null?"0":layer).toUpperCase(Locale.ROOT);}
    private DxfColor(){}
}
