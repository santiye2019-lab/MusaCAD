package com.musa.cad;

/** Resolves AutoCAD Color Index (ACI) and DXF true-color values without Android dependencies. */
public final class DxfColor {
    public static final int BYBLOCK=0;
    public static final int BYLAYER=256;
    public static final int DEFAULT_ACI=7;

    private static final int[] GRAYS={0x333333,0x505050,0x696969,0x828282,0xBEBEBE,0xFFFFFF};

    /** Returns an opaque Android-compatible ARGB color for a DXF true-color (group 420) value. */
    public static int trueColorArgb(int rgb){return 0xFF000000|(rgb&0x00FFFFFF);}

    /** Returns the canonical dark-model-space display color for ACI 1..255. */
    public static int aciArgb(int aci){return 0xFF000000|aciRgb(aci);}

    /** Returns 0xRRGGBB for ACI 1..255. Color 7 is white for MusaCAD's dark canvas. */
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
            case 8:return 0x808080;
            case 9:return 0xC0C0C0;
            default:
                if(aci>=250)return GRAYS[aci-250];
                int n=aci-10;
                int hueStep=n/10;
                int shade=n%10;
                double hue=hueStep*15.0;
                double saturation=(shade&1)==0?1.0:0.5;
                double value;
                switch(shade/2){
                    case 0:value=1.0;break;
                    case 1:value=0.65;break;
                    case 2:value=0.50;break;
                    case 3:value=0.30;break;
                    default:value=0.15;break;
                }
                return hsvRgb(hue,saturation,value);
        }
    }

    private static int hsvRgb(double hue,double saturation,double value){
        double h=(hue%360.0)/60.0;
        int sector=(int)Math.floor(h);
        double f=h-sector;
        double p=value*(1.0-saturation);
        double q=value*(1.0-saturation*f);
        double t=value*(1.0-saturation*(1.0-f));
        double r,g,b;
        switch(sector){
            case 0:r=value;g=t;b=p;break;
            case 1:r=q;g=value;b=p;break;
            case 2:r=p;g=value;b=t;break;
            case 3:r=p;g=q;b=value;break;
            case 4:r=t;g=p;b=value;break;
            default:r=value;g=p;b=q;break;
        }
        int ri=(int)Math.floor(r*255.0+1e-9);
        int gi=(int)Math.floor(g*255.0+1e-9);
        int bi=(int)Math.floor(b*255.0+1e-9);
        return (ri<<16)|(gi<<8)|bi;
    }

    private DxfColor(){}
}
