package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Set;

/**
 * Fast bounded structural disk readback for staged 2D DXF documents.
 * A successful check proves well-formed section/group boundaries and
 * distinct nonzero saved handles; it is NOT DWG semantic parity.
 */
public final class Dxf2dStructureVerifier {
    private static final long MAX_BYTES=512L*1024*1024;
    private static final int MAX_LINE_LENGTH=1024*1024;
    private static final int MAX_PAIRS=32_000_000;
    private static final int MAX_HANDLES=1_500_000;

    private Dxf2dStructureVerifier(){}

    private static String line(BufferedReader reader)throws IOException{
        StringBuilder value=new StringBuilder(48);
        int c;
        while((c=reader.read())!=-1){
            if(c=='\n')break;
            if(c=='\r')continue;
            if(value.length()>=MAX_LINE_LENGTH)
                throw new IOException("DXF grup satırı fazla uzun");
            value.append((char)c);
        }
        return c==-1&&value.length()==0?null:value.toString();
    }

    public static void verify(File dxf)throws IOException{
        if(dxf==null||!dxf.isFile()||dxf.length()==0||dxf.length()>MAX_BYTES)
            throw new IOException("Geçersiz 2B DXF geçici dosyası");
        DxfSectionGuard sections=new DxfSectionGuard();
        int pairs=0;
        Set<String> seenHandles=new HashSet<>();
        try(BufferedReader reader=new BufferedReader(new InputStreamReader(
                new FileInputStream(dxf),StandardCharsets.ISO_8859_1),128*1024)){
            while(true){
                FileTransfer.checkCancelled();
                String group=line(reader);
                if(group==null)break;
                String value=line(reader);
                // Portable C simultaneously validates pair framing and
                // SECTION/ENDSEC/EOF transitions: one JNI call per pair.
                int packet=sections.consume(group,value,pairs,MAX_PAIRS);
                pairs++;
                int code=DxfSectionGuard.code(packet);
                String item=value.trim();
                // Native C verifies LINE coordinate syntax and completeness
                // in the same state transition, before this record is trusted.
                if(code!=0){
                    if((code==5||code==105)&&
                       DxfSectionGuard.event(packet,DxfSectionGuard.HANDLE_AREA)){
                        String handle=DxfSourceHandlePlan.canonical(item);
                        if(!seenHandles.add(handle))throw new IOException("DXF nesne tutamacı yinelendi");
                        if(seenHandles.size()>MAX_HANDLES)throw new IOException("DXF kimlik sınırı aşıldı");
                    }
                }
            }
        }
        sections.requireFinished();
    }

}
