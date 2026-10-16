package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.math.BigInteger;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * Generates collision-free 64-bit DXF group-5 handles for replacement LINEs.
 *
 * All group-5 and DIMSTYLE group-105 values, including HEADER $HANDSEED,
 * reserve their hexadecimal value. This is a DXF-save identity only: no
 * original binary-DWG handle or source XDATA provenance is inferred.
 */
public final class DxfSourceHandlePlan {
    private static final BigInteger MAX=new BigInteger("FFFFFFFFFFFFFFFF",16);
    private static final BigInteger FLOOR=new BigInteger("FF",16);
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
        BigInteger max=FLOOR;
        boolean header=false,sectionPending=false,expectSeed=false;
        int pairs=0;
        try(BufferedReader in=new BufferedReader(
                new InputStreamReader(new FileInputStream(source),charset),128*1024)){
            while(true){
                String codeLine=in.readLine();
                if(codeLine==null)break;
                String valueLine=in.readLine();
                if(valueLine==null)throw new IOException("Truncated DXF group pair");
                if(++pairs>MAX_GROUP_PAIRS)throw new IOException("Excessive DXF group pairs");
                int code;
                try{code=Integer.parseInt(codeLine.trim());}
                catch(NumberFormatException ex){throw new IOException("Invalid DXF group code",ex);}
                if(code<0||code>1071)throw new IOException("Invalid DXF group code range");
                String value=valueLine.trim();
                if(code==0&&"SECTION".equals(value)){sectionPending=true;header=false;continue;}
                if(sectionPending&&code==2){header="HEADER".equalsIgnoreCase(value);sectionPending=false;continue;}
                if(code==0&&"ENDSEC".equals(value)){header=false;expectSeed=false;}
                if(header&&code==9)expectSeed="$HANDSEED".equalsIgnoreCase(value);
                if(code==5||code==105){
                    BigInteger parsed=parse(value);
                    if(parsed.compareTo(max)>0)max=parsed;
                    if(expectSeed&&code==5)expectSeed=false;
                }
            }
        }
        // One larger than every existing handle and HANDSEED. Reserve the
        // following value for the next entity to avoid allocator reuse.
        BigInteger start=max.add(BigInteger.ONE);
        BigInteger seed=start.add(BigInteger.valueOf(newLines));
        if(seed.compareTo(MAX)>0)throw new IOException("DXF handle space exhausted");
        ArrayList<String> values=new ArrayList<>(newLines);
        for(int i=0;i<newLines;i++)
            values.add(start.add(BigInteger.valueOf(i)).toString(16).toUpperCase(Locale.ROOT));
        return new DxfSourceHandlePlan(values,seed.toString(16).toUpperCase(Locale.ROOT));
    }

    /** Validate before any DXF bytes are written by an emitter. */
    public static String canonical(String raw)throws IOException{
        return parse(raw).toString(16).toUpperCase(Locale.ROOT);
    }
    private static BigInteger parse(String raw)throws IOException{
        if(raw==null||raw.isEmpty()||raw.length()>16)
            throw new IOException("Invalid DXF handle length");
        for(int i=0;i<raw.length();i++){
            char ch=raw.charAt(i);
            if(ch>127||Character.digit(ch,16)<0)
                throw new IOException("Invalid DXF handle hexadecimal value");
        }
        BigInteger value=new BigInteger(raw,16);
        if(value.signum()<=0||value.compareTo(MAX)>0)
            throw new IOException("Invalid DXF handle value");
        return value;
    }

    public int count(){return handles.size();}
    public String handleAt(int index){return handles.get(index);}
    public String nextSeed(){return nextSeed;}
}
