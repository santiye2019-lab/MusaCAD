package com.musa.cad;

/** Resolves DXF AutoCAD Color Index (ACI) and true-color values without Android dependencies. */
public final class DxfColor {
    public static final int BYBLOCK=0;
    public static final int BYLAYER=256;
    public static final int DEFAULT_ACI=7;

    private static final int[] LEVELS={255,189,129,104,79};
    private static final int[] GRAYS={0x333333,0x505050,0x696969,0x828282,0xBEBEBE,0xFFFFFF};

    /** Returns an opaque Android-compatible ARGB color for a DXF true-color (group 420) value. */
    public static int trueColorArgb(int rgb){return 0xFF000000|(rgb&0x00FFFFFF);}

    /** Returns an opaque ARGB color for ACI 1..255 using MusaCAD's pinned LibreDWG palette. */
    public static int aciArgb(int aci){return 0xFF000000|aciRgb(aci);}

    /**
     * Returns 0xRRGGBB for ACI 1..255. This matches the 256-entry rgb_palette
     * used by MusaCAD's pinned LibreDWG revision. ACI is an index rather than
     * an embedded RGB value; DXF group 420 TrueColor is preserved separately.
     */
    public static int aciRgb(int aci){
        if(aci<1||aci>255)aci=DEFAULT_ACI;
        switch(aci){
            case 1:return 0xFF0000;
            case 2:return 0xFFFF00;
            case 3:return 0x00FF00;
            case 4:return 0x00FFFF;
            case 5:return 0x0000FF;
            case 6:return 0xFF00FF;
            case 7:return 0xFFFFFF;
            case 8:return 0x414141;
            case 9:return 0x808080;
            default:
                if(aci>=250)return GRAYS[aci-250];
                int n=aci-10;
                int hueStep=n/10;       // 0..23, 15-degree hue steps
                int shade=n%10;         // five brightness levels, solid/pastel pairs
                int level=LEVELS[shade/2];
                int[] base=hueBase(hueStep);
                int r=base[0]*level/255;
                int g=base[1]*level/255;
                int b=base[2]*level/255;
                if((shade&1)!=0){
                    // LibreDWG's odd ACI entries are a 2/3 blend toward the
                    // current brightness level, e.g. 10 red -> 11 pastel red.
                    r=(r+2*level)/3;g=(g+2*level)/3;b=(b+2*level)/3;
                }
                return (r<<16)|(g<<8)|b;
        }
    }

    private static int[] hueBase(int step){
        int sector=step/4;
        int offset=step%4;
        int rising=offset*255/4;          // 0, 63, 127, 191
        int falling=(4-offset)*255/4;     // 255, 191, 127, 63
        switch(sector){
            case 0:return new int[]{255,rising,0};
            case 1:return new int[]{falling,255,0};
            case 2:return new int[]{0,255,rising};
            case 3:return new int[]{0,falling,255};
            case 4:return new int[]{rising,0,255};
            default:return new int[]{255,0,falling};
        }
    }

    private DxfColor(){}
}
