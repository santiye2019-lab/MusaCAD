package com.musa.cad;

import java.io.File;
import java.io.IOException;

/**
 * Fast bounded structural disk readback for staged 2D DXF documents.
 * A successful check proves well-formed section/group boundaries and
 * distinct nonzero saved handles; it is NOT DWG semantic parity.
 */
public final class Dxf2dStructureVerifier {
    private static final long MAX_BYTES=512L*1024*1024;
    private static final int MAX_PAIRS=32_000_000;
    private static final int MAX_HANDLES=1_500_000;

    private Dxf2dStructureVerifier(){}

    public static void verify(File dxf)throws IOException{
        if(dxf==null||!dxf.isFile()||dxf.length()==0||dxf.length()>MAX_BYTES)
            throw new IOException("Geçersiz 2B DXF geçici dosyası");
        DxfSectionGuard sections=new DxfSectionGuard();
        int pairs=0;
        try(DxfSavedHandleSet seenHandles=new DxfSavedHandleSet(MAX_HANDLES);
            DxfPortableLines reader=new DxfPortableLines(dxf)){
            while(true){
                FileTransfer.checkCancelled();
                String group=reader.line();
                if(group==null)break;
                String value=reader.line();
                // Portable C simultaneously validates pair framing and
                // SECTION/ENDSEC/EOF transitions: one JNI call per pair.
                int packet=sections.consume(group,value,pairs,MAX_PAIRS);
                pairs++;
                int code=DxfSectionGuard.code(packet);
                // Native C verifies LINE coordinate syntax and completeness
                // in the same state transition, before this record is trusted.
                if(code!=0){
                    if((code==5||code==105)&&
                       DxfSectionGuard.event(packet,DxfSectionGuard.HANDLE_AREA)){
                        // One shared C set owns 5/105 hex parsing, normalization,
                        // collision checks and the bounded number of handles.
                        seenHandles.reserve(value);
                    }
                }
            }
        }
        sections.requireFinished();
    }

}
