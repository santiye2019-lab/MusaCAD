package com.musa.cad;

/** Common DXF entity visibility rules shared by buffered and streaming readers. */
public final class DxfVisibility {
    public static boolean invisible(String type,int commonVisibility,int flags70){
        if(commonVisibility!=0)return true; // Common entity group 60: 1 = invisible.
        String t=type==null?"":type.trim().toUpperCase(java.util.Locale.ROOT);
        // ATTRIB/ATTDEF group 70 bit 1 is the AutoCAD "invisible attribute" flag.
        return ("ATTRIB".equals(t)||"ATTDEF".equals(t))&&(flags70&1)!=0;
    }
    private DxfVisibility(){}
}
