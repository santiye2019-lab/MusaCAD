package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Preflight for a one-source-to-two-LINE BREAK2 export.
 *
 * A corrupt or stale SourceRange can otherwise erase unrelated geometry,
 * headers, or even an ENTITIES section boundary. Validate exact original
 * record boundaries before DxfWriter opens the destination OutputStream.
 * This parser is JVM-only and independent of the Android renderer.
 */
public final class DxfBreakSourceBoundaryGuard {
    private static final int MAX_PAIRS=32_000_000;
    private static final long MAX_SOURCE_BYTES=512L*1024*1024;

    private DxfBreakSourceBoundaryGuard(){}

    public static void verify(File original,Charset charset,
                              List<SourceReplacement> replacements,
                              List<SourceRange> removals)throws IOException{
        Map<Integer,List<SourceReplacement>> bySource=new HashMap<>();
        Map<Integer,Integer> ownerForStart=new HashMap<>();
        if(replacements!=null)for(SourceReplacement item:replacements){
            if(item==null)continue;
            if(item.range==null||item.sourceId!=item.range.sourceId)
                throw new IOException("Edited DXF entity has an inconsistent source ID");
            Integer earlier=ownerForStart.putIfAbsent(item.range.startLine,item.sourceId);
            if(earlier!=null&&earlier.intValue()!=item.sourceId)
                throw new IOException("Two source IDs claim one original DXF record");
            bySource.computeIfAbsent(item.sourceId,k->new ArrayList<>()).add(item);
        }
        Map<Integer,SourceRange> splitSources=new HashMap<>();
        for(Map.Entry<Integer,List<SourceReplacement>> e:bySource.entrySet()){
            List<SourceReplacement> parts=e.getValue();
            if(parts.size()==1&&!parts.get(0).breakChild)continue; // ordinary non-BREAK source edits
            if(parts.size()>2)throw new IOException("Ambiguous DXF split fragment count");
            SourceRange range=parts.get(0).range;
            for(SourceReplacement part:parts){
                if(part.edit.type!=CadEdit.Type.LINE||part.range==null||
                   part.range.sourceId!=e.getKey()||
                   part.range.startLine!=range.startLine||
                   part.range.endLineExclusive!=range.endLineExclusive)
                    throw new IOException("BREAK fragments do not share one source LINE");
            }
            if(range.startLine+2!=range.sourceId)
                throw new IOException("BREAK source ID must be the original record header offset");
            int exact=0;
            if(removals!=null)for(SourceRange deletion:removals){
                if(deletion==null)continue;
                boolean overlaps=range.startLine<deletion.endLineExclusive&&
                                 deletion.startLine<range.endLineExclusive;
                if(overlaps){
                    if(deletion.startLine!=range.startLine||
                       deletion.endLineExclusive!=range.endLineExclusive||
                       deletion.sourceId!=range.sourceId)
                        throw new IOException("BREAK source deletion covers neighboring DXF records");
                    exact++;
                }
            }
            if(exact!=1)throw new IOException("BREAK source must be removed exactly once");
            if(splitSources.put(range.startLine,range)!=null)
                throw new IOException("Two split groups target one original DXF LINE");
        }
        if(splitSources.isEmpty())return;
        if(original==null||!original.isFile()||charset==null||
           original.length()>MAX_SOURCE_BYTES)
            throw new IOException("BREAK source DXF preflight unavailable");

        boolean sectionPending=false,inEntities=false;
        int currentRecordStart=-1,lines=0,pairs=0;
        String currentRecordType=null;
        Set<Integer> seen=new HashSet<>();
        try(BufferedReader input=new BufferedReader(new InputStreamReader(
                new FileInputStream(original),charset),128*1024)){
            while(true){
                String codeLine=input.readLine();
                if(codeLine==null)break;
                String valueLine=input.readLine();
                if(valueLine==null)throw new IOException("Truncated DXF group pair");
                if(++pairs>MAX_PAIRS)throw new IOException("DXF source preflight size bound");
                int code;
                try{code=Integer.parseInt(codeLine.trim());}
                catch(NumberFormatException ex){throw new IOException("Invalid DXF group code",ex);}
                if(code<0||code>1071)throw new IOException("Invalid DXF group code range");
                String value=valueLine.trim();
                if(code==0){
                    if(currentRecordStart>=0){
                        SourceRange match=splitSources.get(currentRecordStart);
                        if(match!=null){
                            if(!"LINE".equals(currentRecordType)||
                               match.endLineExclusive!=lines)
                                throw new IOException("BREAK would remove more than its source LINE");
                            seen.add(currentRecordStart);
                        }
                        currentRecordStart=-1;currentRecordType=null;
                    }
                    if("ENDSEC".equals(value)){inEntities=false;sectionPending=false;}
                    else if("SECTION".equals(value)){
                        if(inEntities)throw new IOException("Nested DXF section during BREAK preflight");
                        sectionPending=true;
                    }else if(inEntities){
                        currentRecordStart=lines;currentRecordType=value;
                    }
                }else if(sectionPending&&code==2){
                    inEntities="ENTITIES".equalsIgnoreCase(value);
                    sectionPending=false;
                }
                lines+=2;
            }
        }
        if(currentRecordStart>=0){
            SourceRange match=splitSources.get(currentRecordStart);
            if(match!=null){
                if(!"LINE".equals(currentRecordType)||match.endLineExclusive!=lines)
                    throw new IOException("BREAK source LINE ends outside record");
                seen.add(currentRecordStart);
            }
        }
        if(seen.size()!=splitSources.size())
            throw new IOException("BREAK source LINE record was not found in ENTITIES");
    }
}
