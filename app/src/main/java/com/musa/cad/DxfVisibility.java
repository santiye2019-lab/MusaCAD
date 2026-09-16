package com.musa.cad;

/** Common DXF entity visibility rules shared by buffered and streaming readers. */
public final class DxfVisibility {
    public static boolean invisible(String type,int commonVisibility,int flags70){
        if(commonVisibility!=0)return true; // Common entity group 60: 1 = invisible.
        String t=type==null?"":type.trim().toUpperCase(java.util.Locale.ROOT);
        if("ATTRIB".equals(t))return (flags70&1)!=0; // Attribute reference: bit 1 = invisible.
        if("ATTDEF".equals(t)){
            // ATTDEF is a block-template entity. Variable definitions are replaced by the
            // INSERT's following ATTRIB references and must not be drawn as duplicate default text.
            // Constant definitions (bit 2) remain part of the inserted block graphics.
            return (flags70&1)!=0||(flags70&2)==0;
        }
        return false;
    }
    private DxfVisibility(){}
}
