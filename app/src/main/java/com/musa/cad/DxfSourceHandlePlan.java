package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Generates collision-free 64-bit DXF group-5 handles for replacement LINEs.
 *
 * All group-5 and DIMSTYLE group-105 values, including HEADER $HANDSEED,
 * reserve their hexadecimal value. This is a DXF-save identity only: no
 * original binary-DWG handle or source XDATA provenance is inferred.
 */
public final class DxfSourceHandlePlan {
    // Only the portable C core validates, normalizes or allocates DXF identities.
    private static final boolean sharedCoreLoaded;
    static {
        boolean loaded=false;
        try {
            System.loadLibrary("musacad_dwg");
            loaded=true;
        } catch (LinkageError | SecurityException unavailable) {
            // A missing common core must STOP saving, not generate duplicate handles.
        }
        sharedCoreLoaded=loaded;
    }
    private static native String canonicalNative(String raw);
    private static native String maximumNative(String maximum,String raw);
    private static native String[] allocateNative(String highest,int count);
    private static void requireCore()throws IOException{
        if(!sharedCoreLoaded)
            throw new IOException("Ortak CAD çekirdeği olmadan DXF nesne kimliği doğrulanamaz");
    }
    private static final int MAX_GROUP_PAIRS=32_000_000;
    private final List<String> handles;
    private final String nextSeed;

    private DxfSourceHandlePlan(List<String> handles,String nextSeed){
        this.handles=Collections.unmodifiableList(handles);
        this.nextSeed=nextSeed;
    }

    public static DxfSourceHandlePlan prepare(File source,Charset charset,int newLines)
            throws IOException{
        if(source==null||!source.isFile()||charset==null||newLines<1||newLines>100_000)
            throw new IOException("DXF LINE identity plan arguments invalid");
        if(source.length()>512L*1024L*1024L)
            throw new IOException("DXF handle source exceeds bounded input");
        requireCore();
        // All group-5/105 values, including HEADER $HANDSEED, are reserved
        // unconditionally. The previous Java section flags were unused.
        String maximum="FF";
        int pairs=0;
        try(BufferedReader in=new BufferedReader(
                new InputStreamReader(new FileInputStream(source),charset),128*1024)){
            while(true){
                String codeLine=in.readLine();
                if(codeLine==null)break;
                String valueLine=in.readLine();
                int code=DxfGroupCode.parsePair(codeLine,valueLine,pairs,MAX_GROUP_PAIRS);
                pairs++;
                String value=valueLine.trim();
                if(code==5||code==105){
                    try {
                        maximum=maximumNative(maximum,value);
                    } catch (LinkageError unavailable) {
                        throw new IOException("Ortak CAD çekirdeğinde DXF kimlik işlevi yok",unavailable);
                    }
                    if(maximum==null)throw new IOException("Invalid DXF handle hexadecimal value");
                }
            }
        }
        // Common C uint64 allocator reserves each child and the next
        // HANDSEED, with explicit unsigned overflow rejection.
        final String[] allocated;
        try {
            allocated=allocateNative(maximum,newLines);
        } catch(LinkageError unavailable) {
            throw new IOException("Ortak CAD çekirdeğinde kimlik ayırıcı bulunamadı",unavailable);
        }
        if(allocated==null||allocated.length!=newLines+1)
            throw new IOException("DXF handle space exhausted");
        ArrayList<String> values=new ArrayList<>(newLines);
        for(int i=0;i<newLines;i++)values.add(allocated[i]);
        return new DxfSourceHandlePlan(values,allocated[newLines]);
    }

    /** Validate in shared native core before ANY DXF entity bytes are published. */
    public static String canonical(String raw)throws IOException{
        requireCore();
        final String result;
        try {
            result=canonicalNative(raw);
        } catch(LinkageError unavailable) {
            throw new IOException("Ortak CAD çekirdeğinde kimlik doğrulayıcı bulunamadı",unavailable);
        }
        if(result==null)throw new IOException("Invalid DXF handle hexadecimal value");
        return result;
    }

    public int count(){return handles.size();}
    public String handleAt(int index){return handles.get(index);}
    public String nextSeed(){return nextSeed;}

}
