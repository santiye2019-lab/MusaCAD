package com.musa.cad;

/** Pure-Java helpers for TEXT/MTEXT alignment semantics. */
public final class DxfTextLayout {
    /** Horizontal anchor fraction: 0=left, .5=center, 1=right. */
    public static double textHorizontal(int code){
        if(code==1||code==4)return .5d;
        if(code==2)return 1d;
        return 0d;
    }
    /** Vertical anchor: 0=baseline, 1=bottom, 2=middle, 3=top. */
    public static int textVertical(int code){return code>=1&&code<=3?code:0;}
    /** MTEXT attachment horizontal fraction for codes 1..9. */
    public static double mtextHorizontal(int attachment){
        int a=clampAttachment(attachment),column=(a-1)%3;
        return column==0?0d:column==1?.5d:1d;
    }
    /** MTEXT attachment row: 0=top, 1=middle, 2=bottom. */
    public static int mtextVertical(int attachment){return (clampAttachment(attachment)-1)/3;}
    public static boolean usesAlignmentPoint(int hAlign,int vAlign){return hAlign!=0||vAlign!=0;}
    public static boolean isAlignedOrFit(int hAlign){return hAlign==3||hAlign==5;}
    private static int clampAttachment(int a){return a>=1&&a<=9?a:1;}
    private DxfTextLayout(){}
}
