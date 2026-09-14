package com.musa.cad;

import java.io.*;

/** Offline conversion; caller retains the original DWG and a temporary DXF working copy. */
public final class NativeDwg {
    static {System.loadLibrary("musacad_dwg");}
    private static native int convertNative(String input,String output);

    public static final class Conversion {
        public final DxfParser.Result result;
        public final File dxf;
        public final int warnings;
        Conversion(DxfParser.Result result,File dxf,int warnings){this.result=result;this.dxf=dxf;this.warnings=warnings;}
    }

    public static Conversion read(File dwg,File cache)throws IOException {
        FileTransfer.checkCancelled();
        File converted=File.createTempFile("MusaCAD_donusen_",".dxf",cache);
        boolean keep=false;
        try{
            int status=convertNative(dwg.getAbsolutePath(),converted.getAbsolutePath());
            FileTransfer.checkCancelled();
            if(status<0)throw new IOException("DWG dönüşümü başarısız ("+status+")");
            if(converted.length()>256L*1024*1024)throw new IOException("Dönüştürülen çizim 256 MB sınırını aşıyor");
            DxfParser.Result result=DxfParser.render(converted);
            if(result==null)throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı");
            result.conversionWarnings=status;keep=true;
            return new Conversion(result,converted,status);
        }finally{if(!keep)converted.delete();}
    }
    private NativeDwg(){}
}
