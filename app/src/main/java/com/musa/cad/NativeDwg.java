package com.musa.cad;

import java.io.*;

/** Offline DWG conversion. Viewing can use read(); editor flows can retain the converted DXF via readWithWorkingCopy(). */
public final class NativeDwg {
    static {System.loadLibrary("musacad_dwg");}
    private static native int convertNative(String input,String output);

    public static final class Conversion {
        public final DxfParser.Result result;
        public final File dxf;
        public final int warnings;
        public final NativeCadEngine.Stats nativeStats;
        Conversion(DxfParser.Result result,File dxf,int warnings,NativeCadEngine.Stats nativeStats){
            this.result=result;this.dxf=dxf;this.warnings=warnings;this.nativeStats=nativeStats;
        }
    }

    /** Backward-compatible viewing path used by the current MainActivity. */
    public static DxfParser.Result read(File dwg,File cache)throws IOException {
        Conversion conversion=readWithWorkingCopy(dwg,cache);
        try{return conversion.result;}
        finally{if(conversion.dxf!=null)conversion.dxf.delete();}
    }

    /**
     * Stable editor path. The DWG is now opened through NativeCadEngine first,
     * so direct viewport queries can be introduced incrementally without
     * changing the existing DXF editor/render contract.
     */
    public static Conversion readWithWorkingCopy(File dwg,File cache)throws IOException {
        FileTransfer.checkCancelled();
        File converted=File.createTempFile("MusaCAD_donusen_",".dxf",cache);
        boolean keep=false;
        try(NativeCadEngine engine=NativeCadEngine.open(dwg)){
            NativeCadEngine.Stats nativeStats=engine.stats();
            int status=engine.exportDxf(converted);
            FileTransfer.checkCancelled();
            if(converted.length()>256L*1024*1024)throw new IOException("Dönüştürülen çizim 256 MB sınırını aşıyor");
            DxfParser.Result result=DxfParser.render(converted);
            if(result==null)throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı");
            result.conversionWarnings=status;keep=true;
            return new Conversion(result,converted,status,nativeStats);
        }finally{if(!keep)converted.delete();}
    }
    private NativeDwg(){}
}
