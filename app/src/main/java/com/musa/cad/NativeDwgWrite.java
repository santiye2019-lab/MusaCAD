package com.musa.cad;

import java.io.File;
import java.io.IOException;

/**
 * Experimental LibreDWG legacy (r2000-r2004) writer.
 * Disabled in normal builds; unsupported/future formats are explicitly rejected.
 * An output must reopen and preserve native object count before it is returned.
 */
public final class NativeDwgWrite {
    static {System.loadLibrary("musacad_dwg");}
    private static native boolean availableNative();
    private static native int exportNative(String stagedDxf,String candidateDwg);
    private NativeDwgWrite(){}

    public static boolean available(){
        try{return availableNative();}catch(LinkageError ignored){return false;}
    }
    public static File exportStaged(File stagedDxf,File cacheDir)throws IOException{
        if(!available())throw new IOException(
            "Deneysel DWG yazıcısı bu APK'da etkin değil; DXF dışa aktarımını kullanın");
        if(stagedDxf==null||!stagedDxf.isFile()||cacheDir==null||!cacheDir.isDirectory())
            throw new IOException("DWG yazma için doğrulanmış geçici DXF bulunamadı");
        CadDxfSaveStage.validate(stagedDxf);
        File temp=File.createTempFile("MusaCAD_dwg_legacy_",".dwg",cacheDir);
        // LibreDWG deliberately refuses to overwrite *any* existing file.
        // Reserve an unpredictable private-cache name, then unlink it before
        // passing the output path to its writer.
        if(!temp.delete())throw new IOException("DWG geçici dosyası hazırlanamadı");
        boolean valid=false;
        try{
            // LibreDWG writer must not overwrite/truncate the user's original file.
            int status=exportNative(stagedDxf.getAbsolutePath(),temp.getAbsolutePath());
            if(status!=0){
                String problem=status==-93
                    ?"DWG 2007/2010/2013/2018 kaydı bu deneysel yazıcıda güvenli değildir"
                    :status==-90?"DWG yazma desteği derlemede kapalı":"LibreDWG yazma/doğrulama hatası ("+status+")";
                throw new IOException(problem);
            }
            if(temp.length()<128||temp.length()>512L*1024L*1024L)
                throw new IOException("Oluşturulan DWG boş veya sınır dışında");
            try(NativeCadEngine reopened=NativeCadEngine.open(temp)){
                NativeCadEngine.Stats stats=reopened.stats();
                if(stats.objectCount<=0||stats.entityCount<=0)
                    throw new IOException("Üretilen DWG'de okunabilir geometri doğrulanamadı");
            }
            valid=true;
            return temp;
        }finally{
            if(!valid&&!temp.delete()&&temp.exists())temp.deleteOnExit();
        }
    }
}
