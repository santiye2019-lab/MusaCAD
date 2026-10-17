package com.musa.cad;

import java.io.IOException;

/**
 * Strict staged-DXF SECTION/ENDSEC/EOF validation lives ONLY in portable C.
 * Platform code retains file I/O and handle/LINE fixture readback.
 *
 * The native call combines existing group-code pair validation with section
 * state transitions, avoiding two JNI round trips for every DXF group pair.
 */
public final class DxfSectionGuard {
    public static final int RECORD_BOUNDARY=1;
    public static final int LINE_STARTED=2;
    public static final int HANDLE_AREA=4;
    private static final int CODE_MASK=2047;
    private final int[] state=new int[4];

    private static final boolean sharedCoreLoaded;
    static {
        boolean available=false;
        try {
            System.loadLibrary("musacad_dwg");
            available=true;
        }catch(LinkageError | SecurityException missing) {
            // Never accept a DXF record using an independent Java CAD parser.
        }
        sharedCoreLoaded=available;
    }
    private static native int consumeNative(int[] state,String groupCode,
                                            String value,int count,int maximum);
    private static native int finishNative(int[] state);

    /** Packed nonnegative result: low 11 bits are group code; upper are flags. */
    public int consume(String groupCode,String value,int count,int maximum)
            throws IOException{
        requireCore();
        final int packed;
        try{
            packed=consumeNative(state,groupCode,value,count,maximum);
        }catch(LinkageError missing){
            throw new IOException("Ortak CAD çekirdeğinde DXF bölüm okuyucusu yok",missing);
        }
        if(packed<0)throw new IOException(messageFor(packed));
        return packed;
    }

    public void requireFinished()throws IOException{
        requireCore();
        final int result;
        try{
            result=finishNative(state);
        }catch(LinkageError missing){
            throw new IOException("Ortak CAD çekirdeğinde DXF bölüm bitiş kontrolü yok",missing);
        }
        if(result!=0)throw new IOException(messageFor(result));
    }
    public static int code(int packet){return packet&CODE_MASK;}
    public static boolean event(int packet,int flag){
        return (packet&(flag<<11))!=0;
    }
    private static void requireCore()throws IOException{
        if(!sharedCoreLoaded)
            throw new IOException("Ortak CAD çekirdeği olmadan DXF bölüm doğrulanamaz");
    }
    private static String messageFor(int status){
        switch(status){
            case -1:return "Eksik DXF grup çifti";
            case -2:return "Geçersiz DXF grup kodu";
            case -3:return "DXF grup sayısı fazla";
            case -4:return "DXF grup çifti parametreleri geçersiz";
            case -10:return "DXF EOF sonrasında veri bulunuyor";
            case -11:return "DXF SECTION adı eksik";
            case -12:return "Çift DXF ENTITIES bölümü";
            case -13:return "İç içe DXF SECTION";
            case -14:return "DXF ENDSEC eşleşmiyor";
            case -15:return "DXF EOF veya ENTITIES eksik";
            case -16:return "DXF nesnesi SECTION dışında";
            case -17:return "DXF kaydı tamamlanmamış";
            default:return "DXF bölüm kaydı geçersiz veya çekirdek hatası";
        }
    }
}
