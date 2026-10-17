package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Locale;
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
        String section=null;
        boolean sectionPending=false,seenEntities=false,seenEnd=false;
        boolean entityIsLine=false;
        int lineFlags=0,pairs=0;
        Set<String> seenHandles=new HashSet<>();
        try(BufferedReader reader=new BufferedReader(new InputStreamReader(
                new FileInputStream(dxf),StandardCharsets.ISO_8859_1),128*1024)){
            while(true){
                FileTransfer.checkCancelled();
                String group=line(reader);
                if(group==null)break;
                String value=line(reader);
                if(value==null)throw new IOException("Eksik DXF grup çifti");
                if(++pairs>MAX_PAIRS)throw new IOException("DXF grup sayısı fazla");
                int code=DxfGroupCode.parse(group);
                String item=value.trim();
                if(seenEnd)throw new IOException("DXF EOF sonrasında veri bulunuyor");
                if(sectionPending){
                    if(code!=2||item.isEmpty())
                        throw new IOException("DXF SECTION adı eksik");
                    section=item.toUpperCase(Locale.ROOT);
                    if("ENTITIES".equals(section)){
                        if(seenEntities)throw new IOException("Çift DXF ENTITIES bölümü");
                        seenEntities=true;
                    }
                    sectionPending=false;
                    continue;
                }
                if(code==0){
                    if(entityIsLine){
                        requireLine(lineFlags);
                        entityIsLine=false;lineFlags=0;
                    }
                    if("SECTION".equals(item)){
                        if(section!=null)throw new IOException("İç içe DXF SECTION");
                        sectionPending=true;
                    }else if("ENDSEC".equals(item)){
                        if(section==null)throw new IOException("DXF ENDSEC eşleşmiyor");
                        section=null;
                    }else if("EOF".equals(item)){
                        if(section!=null||!seenEntities)
                            throw new IOException("DXF EOF veya ENTITIES eksik");
                        seenEnd=true;
                    }else{
                        if(section==null)throw new IOException("DXF nesnesi SECTION dışında");
                        if("ENTITIES".equals(section)&&"LINE".equals(item)){
                            entityIsLine=true;lineFlags=0;
                        }
                    }
                }else{
                    if(section==null)throw new IOException("DXF değer SECTION dışında");
                    if(entityIsLine){
                        int bit=code==10?1:code==20?2:code==11?4:code==21?8:0;
                        if(bit!=0){
                            try{if(!Double.isFinite(Double.parseDouble(item)))
                                throw new NumberFormatException("non-finite");}
                            catch(NumberFormatException ex){
                                throw new IOException("DXF LINE sayısal koordinatı geçersiz",ex);
                            }
                            lineFlags|=bit;
                        }
                    }
                    if((code==5||code==105)&&
                       ("ENTITIES".equals(section)||"BLOCKS".equals(section)||
                        "TABLES".equals(section)||"OBJECTS".equals(section))){
                        String handle=DxfSourceHandlePlan.canonical(item);
                        if(!seenHandles.add(handle))throw new IOException("DXF nesne tutamacı yinelendi");
                        if(seenHandles.size()>MAX_HANDLES)throw new IOException("DXF kimlik sınırı aşıldı");
                    }
                }
            }
        }
        if(sectionPending||section!=null||!seenEntities||!seenEnd)
            throw new IOException("DXF kaydı tamamlanmamış");
    }

    private static void requireLine(int flags)throws IOException{
        if((flags&15)!=15)throw new IOException("Kaydedilmiş LINE koordinatları eksik");
    }
}
