package com.musa.cad;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Independent on-disk DXF group-code semantic readback for edited source LINEs.
 * Verifies the actual saved handles, geometry and independent BREAK child
 * styles, not just a syntactically readable DXF document. DWG binary identity
 * and third-party AutoCAD equivalence remain out of scope.
 *
 * MUST run after Dxf2dStructureVerifier.verify(stage), which bounds lines and
 * checks duplicate handles over all DXF sections.
 */
public final class Dxf2dSavedLineReopenProof {
    private Dxf2dSavedLineReopenProof(){}

    public static final class Expected {
        public final String handle,layout;
        public final SourceReplacement replacement;
        public final double x0,y0,x1,y1;
        public Expected(String handle,String layout,SourceReplacement replacement,
                        double x0,double y0,double x1,double y1)throws IOException{
            if(replacement==null||replacement.edit.type!=CadEdit.Type.LINE)
                throw new IOException("DXF LINE readback expectation invalid");
            this.handle=DxfSourceHandlePlan.canonical(handle);
            this.layout=layout==null||layout.trim().isEmpty()?"Model":layout.trim();
            this.replacement=replacement.copy();
            this.x0=x0;this.y0=y0;this.x1=x1;this.y1=y1;
            if(!Double.isFinite(x0)||!Double.isFinite(y0)||
               !Double.isFinite(x1)||!Double.isFinite(y1))
                throw new IOException("Non-finite expected LINE coordinates");
        }
    }

    public static void verify(File stage,Charset charset,List<Expected> expected)
            throws IOException{
        verify(stage,charset,expected,Collections.emptySet());
    }

    /** Retired source handles must not survive as ENTITIES after a BREAK edit. */
    public static void verify(File stage,Charset charset,List<Expected> expected,
                              Set<String> retiredHandles)throws IOException{
        if((expected==null||expected.isEmpty())&&
           (retiredHandles==null||retiredHandles.isEmpty()))return;
        if(stage==null||!stage.isFile()||charset==null||stage.length()>512L*1024*1024)
            throw new IOException("Saved LINE readback file unavailable");
        Map<String,Expected> required=new HashMap<>();
        if(expected!=null)for(Expected e:expected){
            if(e==null||required.putIfAbsent(e.handle,e)!=null)
                throw new IOException("Duplicate expected DXF LINE handle");
        }
        Set<String> retired=new HashSet<>();
        if(retiredHandles!=null)for(String oldHandle:retiredHandles){
            String h=DxfSourceHandlePlan.canonical(oldHandle);
            if(required.containsKey(h))
                throw new IOException("Retired source handle reused by BREAK child");
            retired.add(h);
        }
        Set<String> seen=new HashSet<>();
        boolean pending=false,inEntities=false;
        String type=null;
        Map<Integer,String> groups=new HashMap<>();
        int totalPairs=0;
        try(BufferedReader reader=new BufferedReader(
                new InputStreamReader(new FileInputStream(stage),charset),128*1024)){
            while(true){
                String raw=reader.readLine();
                if(raw==null)break;
                String value=reader.readLine();
                if(value==null)throw new IOException("Truncated reopened DXF");
                if(++totalPairs>32_000_000)throw new IOException("Reopen DXF size limit");
                int code;
                try{code=Integer.parseInt(raw.trim());}
                catch(NumberFormatException error){
                    throw new IOException("Invalid reopened DXF group",error);
                }
                String data=value.trim();
                if(code==0){
                    if(type!=null){checkRecord(type,groups,required,retired,seen);groups.clear();type=null;}
                    if("SECTION".equals(data)){pending=true;inEntities=false;}
                    else if("ENDSEC".equals(data))inEntities=false;
                    else if(inEntities)type=data;
                }else if(pending&&code==2){
                    inEntities="ENTITIES".equalsIgnoreCase(data);pending=false;
                }else if(inEntities&&type!=null){
                    if(groups.putIfAbsent(code,data)!=null&&
                       (code==5||code==8||code==6||code==62||code==420||
                        code==10||code==20||code==11||code==21||
                        code==370||code==48||code==410||code==67))
                        throw new IOException("Duplicate saved LINE property group");
                }
            }
        }
        if(type!=null)checkRecord(type,groups,required,seen);
        if(seen.size()!=required.size())
            throw new IOException("Saved DXF missing generated LINE handle(s)");
    }

    /**
     * Extract only handles of original BREAK source LINE records from the
     * verified source ranges; source LINEs without group-5 have no retired
     * handle to compare. Never infer a DWG binary object identity here.
     */
    public static Set<String> retiredBreakSourceHandles(File original,Charset charset,
            List<SourceReplacement> replacements)throws IOException{
        Map<Integer,SourceRange> ranges=new HashMap<>();
        if(replacements!=null)for(SourceReplacement item:replacements){
            if(item==null||!item.breakChild)continue;
            SourceRange r=item.range;
            if(r==null||item.sourceId!=r.sourceId||r.startLine+2!=r.sourceId)
                throw new IOException("Invalid original BREAK source range");
            SourceRange previous=ranges.putIfAbsent(r.startLine,r);
            if(previous!=null&&(previous.sourceId!=r.sourceId||
                previous.endLineExclusive!=r.endLineExclusive))
                throw new IOException("Conflicting original BREAK source ranges");
        }
        if(ranges.isEmpty())return Collections.emptySet();
        if(original==null||!original.isFile()||charset==null||
           original.length()>512L*1024*1024)
            throw new IOException("Original BREAK source DXF unavailable");
        Set<String> retired=new HashSet<>();
        boolean pending=false,inEntities=false;
        SourceRange active=null;
        int lineIndex=0,pairs=0;
        try(BufferedReader reader=new BufferedReader(
                new InputStreamReader(new FileInputStream(original),charset),128*1024)){
            while(true){
                String raw=reader.readLine();
                if(raw==null)break;
                String value=reader.readLine();
                if(value==null)throw new IOException("Truncated BREAK source DXF");
                if(++pairs>32_000_000)throw new IOException("BREAK source size limit");
                int code;
                try{code=Integer.parseInt(raw.trim());}
                catch(NumberFormatException error){
                    throw new IOException("Invalid BREAK source group code",error);
                }
                String data=value.trim();
                if(code==0){
                    active=null;
                    if("SECTION".equals(data)){pending=true;inEntities=false;}
                    else if("ENDSEC".equals(data)){pending=false;inEntities=false;}
                    else if(inEntities&&"LINE".equals(data))active=ranges.get(lineIndex);
                }else if(pending&&code==2){
                    inEntities="ENTITIES".equalsIgnoreCase(data);pending=false;
                }else if(active!=null&&code==5&&active.containsLine(lineIndex)){
                    if(!retired.add(DxfSourceHandlePlan.canonical(data)))
                        throw new IOException("Duplicate original BREAK LINE handle");
                }
                lineIndex+=2;
            }
        }
        return retired;
    }

    private static void checkRecord(String type,Map<Integer,String> fields,
                                    Map<String,Expected> required,Set<String> retired,
                                    Set<String> seen)throws IOException{
        String raw=fields.get(5);
        if(raw==null)return;
        // Unknown handles belong to existing entities and must remain untouched.
        String handle=DxfSourceHandlePlan.canonical(raw);
        if(retired.contains(handle))
            throw new IOException("Retired original BREAK LINE remains in saved DXF");
        Expected e=required.get(handle);
        if(e==null)return;
        if(!"LINE".equals(type)||!seen.add(handle))
            throw new IOException("Saved DXF LINE identity changed or duplicated");
        SourceReplacement r=e.replacement;
        equals(fields,8,r.layer);
        equals(fields,6,DxfLineStyle.normalizeName(r.lineType));
        equals(fields,370,Integer.toString(DxfLineStyle.normalizeWeight(
                     r.lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT)));
        if(r.colorMode==SourceReplacement.COLOR_BYLAYER){
            if(fields.containsKey(62)||fields.containsKey(420))
                throw new IOException("Saved BYLAYER LINE contains explicit color");
        }else if(r.colorMode==SourceReplacement.COLOR_BYBLOCK){
            equals(fields,62,"0");
            if(fields.containsKey(420))
                throw new IOException("Saved BYBLOCK LINE has TrueColor");
        }else{
            equals(fields,420,Integer.toString(r.color&0x00FFFFFF));
            if(fields.containsKey(62))
                throw new IOException("Saved TrueColor LINE has unexpected ACI");
        }
        number(fields,10,e.x0);number(fields,20,e.y0);
        number(fields,11,e.x1);number(fields,21,e.y1);
        number(fields,30,0d);number(fields,31,0d);
        number(fields,48,r.lineTypeScale);
        if("Model".equalsIgnoreCase(e.layout)){
            if(fields.containsKey(410)||fields.containsKey(67))
                throw new IOException("Saved Model LINE leaked into paper space");
        }else{
            equals(fields,410,e.layout);equals(fields,67,"1");
        }
    }
    private static void equals(Map<Integer,String> fields,int code,String expected)
            throws IOException{
        if(!expected.equals(fields.get(code)))
            throw new IOException("Saved DXF LINE group "+code+" changed");
    }
    private static void number(Map<Integer,String> fields,int code,double expected)
            throws IOException{
        String raw=fields.get(code);
        if(raw==null){
            if(code==48){ // DXF default scale
                if(Math.abs(expected-1d)<=1e-9)return;
            }
            throw new IOException("Saved DXF LINE number missing "+code);
        }
        final double actual;
        try{actual=Double.parseDouble(raw);}
        catch(NumberFormatException x){throw new IOException("Invalid saved LINE number "+code,x);}
        if(!Double.isFinite(actual)||Math.abs(actual-expected)>1e-6)
            throw new IOException("Saved DXF LINE numeric value changed "+code);
    }
}
