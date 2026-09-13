package com.musa.cad;

import java.io.*;

/** Offline conversion; caller retains the original DWG for sharing. */
public final class NativeDwg {
    static {System.loadLibrary("musacad_dwg");}
    private static native int convertNative(String input,String output);
    public static DxfParser.Result read(File dwg,File cache)throws IOException {
        FileTransfer.checkCancelled();
        File converted=File.createTempFile("MusaCAD_donusen_",".dxf",cache);
        try{
            int status=convertNative(dwg.getAbsolutePath(),converted.getAbsolutePath());
            FileTransfer.checkCancelled();
            if(status<0)throw new IOException("DWG dönüşümü başarısız ("+status+")");
            if(converted.length()>DxfStream.MAX_BYTES)throw new IOException("Dönüştürülen çizim 512 MB sınırını aşıyor");
            DxfParser.Result result=DxfParser.render(converted);
            if(result==null)throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı");
            result.conversionWarnings=status;return result;
        }finally{converted.delete();}
    }
    private NativeDwg(){}
}
