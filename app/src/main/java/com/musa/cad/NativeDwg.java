package com.musa.cad;

import java.io.*;

/** Offline conversion; caller retains the original DWG for sharing. */
public final class NativeDwg {
    static {System.loadLibrary("musacad_dwg");}
    private static native int convertNative(String input,String output);
    public static DxfParser.Result read(File dwg,File cache)throws IOException {return read(dwg,cache,null);}

    public static DxfParser.Result read(File dwg,File cache,String preferredLayout)throws IOException {
        FileTransfer.checkCancelled();
        File converted=File.createTempFile("MusaCAD_donusen_",".dxf",cache);
        File flattened=null;
        try{
            int status=convertNative(dwg.getAbsolutePath(),converted.getAbsolutePath());
            FileTransfer.checkCancelled();
            if(status<0)throw new IOException("DWG dönüşümü başarısız ("+status+")");
            // Büyük DWG dosyaları dönüşüm sırasında çok daha büyük ASCII DXF üretebilir.
            // Sabit MB sınırı uygulamıyoruz; DxfParser büyük dosyalarda streaming moda geçer.
            flattened=DxfDimensionFlattener.flatten(converted,cache);
            DxfParser.Result result=DxfParser.render(flattened,preferredLayout);
            if(result==null)throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı");
            result.conversionWarnings=status;return result;
        }finally{
            if(flattened!=null&&flattened!=converted)flattened.delete();
            converted.delete();
        }
    }
    private NativeDwg(){}
}
