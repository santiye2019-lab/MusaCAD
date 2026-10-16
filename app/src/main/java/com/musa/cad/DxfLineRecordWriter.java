package com.musa.cad;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.Locale;

/**
 * Writes one actual DXF LINE entity for an edited source entity, including
 * its own per-child style. Pure Java so the identical writer used by
 * DxfWriter can be verified without Android UI or generating an APK.
 *
 * Coordinates must already be mapped from drawing-content to DXF world
 * coordinates by the caller. DWG binary handles/XDATA are out of scope.
 */
public final class DxfLineRecordWriter {
    private DxfLineRecordWriter(){}

    public static void write(BufferedWriter out,SourceReplacement source,
                             String layout,double x0,double y0,double x1,double y1)
            throws IOException{
        write(out,source,layout,x0,y0,x1,y1,null);
    }

    /** Optional unique group-5 handle from DxfSourceHandlePlan. */
    public static void write(BufferedWriter out,SourceReplacement source,
                             String layout,double x0,double y0,double x1,double y1,
                             String savedHandle)throws IOException{
        if(out==null||source==null||source.edit.type!=CadEdit.Type.LINE)
            throw new IOException("DXF LINE source replacement missing");
        // Validate everything BEFORE emitting the first group code.
        String[] coords={DxfNumber.format(x0),DxfNumber.format(y0),
                         DxfNumber.format(x1),DxfNumber.format(y1)};
        if(Math.hypot(x1-x0,y1-y0)<=1e-12)
            throw new IOException("DXF LINE source replacement has zero length");
        String layer=clean(source.layer,"layer");
        String lineType=clean(DxfLineStyle.normalizeName(source.lineType),"linetype");
        String targetLayout=layout==null||layout.trim().isEmpty()?"Model":
                            clean(layout.trim(),"layout");
        int weight=DxfLineStyle.normalizeWeight(
            source.lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT);
        String lineScale=DxfNumber.format(source.lineTypeScale);
        int color=source.color&0x00FFFFFF;
        String handle=savedHandle==null?null:DxfSourceHandlePlan.canonical(savedHandle);

        tag(out,0,"LINE");
        if(handle!=null)tag(out,5,handle);
        tag(out,8,layer);
        if(!"Model".equalsIgnoreCase(targetLayout)){
            tag(out,67,"1");
            tag(out,410,targetLayout);
        }
        if(source.colorMode==SourceReplacement.COLOR_BYBLOCK)tag(out,62,"0");
        else if(source.colorMode==SourceReplacement.COLOR_EXPLICIT)
            tag(out,420,Integer.toString(color));
        // COLOR_BYLAYER correctly omits entity-level color group codes.
        tag(out,6,lineType);
        if(Math.abs(source.lineTypeScale-1d)>1e-9)tag(out,48,lineScale);
        tag(out,370,Integer.toString(weight));
        tag(out,10,coords[0]);tag(out,20,coords[1]);tag(out,30,"0");
        tag(out,11,coords[2]);tag(out,21,coords[3]);tag(out,31,"0");
    }

    private static String clean(String value,String what)throws IOException{
        if(value==null||value.trim().isEmpty()||value.indexOf('\r')>=0||
           value.indexOf('\n')>=0||value.length()>255)
            throw new IOException("Invalid DXF "+what);
        return value;
    }

    private static void tag(BufferedWriter writer,int code,String value)
            throws IOException{
        writer.write(String.format(Locale.US,"%3d",code));
        writer.newLine();writer.write(value);writer.newLine();
    }
}
