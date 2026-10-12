package com.musa.cad;

import java.util.List;

/**
 * Compact diagnostics for SAT/SAB-bearing DXF entities. This class audits
 * geometry transport; it does not tessellate ACIS B-reps and does not count
 * an entity tag as proof that its shape is available for rendering.
 *
 * Incremental: called by DxfBlocks during its existing streaming pass, so
 * even large DXF files need no extra whole-file parse and ACIS payloads are
 * never retained in application memory.
 */
public final class DxfAcisCoverage {
    private int solid, region, body, withInlineData, withoutInlineData, badHexChunks;
    private long inlineTextChars, inlineBinaryBytes;

    public void accept(String entityType,List<String> tags,int from,int to) {
        if(!"3DSOLID".equals(entityType)&&!"REGION".equals(entityType)&&!"BODY".equals(entityType))return;
        if("3DSOLID".equals(entityType))solid++;
        else if("REGION".equals(entityType))region++;
        else body++;

        boolean payload=false, invalid=false;
        for(int i=from;i+1<to;i+=2){
            int code;
            try{code=Integer.parseInt(tags.get(i).trim());}
            catch(RuntimeException ex){continue;}
            String data=tags.get(i+1);
            if(data==null||data.isEmpty())continue;
            if(code==1||code==3) {
                payload=true;
                inlineTextChars+=data.length();
            }else if(code==310) {
                if((data.length()&1)!=0){invalid=true;badHexChunks++;continue;}
                boolean hex=true;
                for(int j=0;j<data.length();j++){
                    char ch=data.charAt(j);
                    if(!((ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F'))){
                        hex=false;break;
                    }
                }
                if(!hex){invalid=true;badHexChunks++;continue;}
                payload=true;inlineBinaryBytes+=data.length()/2;
            }
        }
        if(payload&&!invalid)withInlineData++;
        else withoutInlineData++;
    }

    public Snapshot snapshot(){
        return new Snapshot(solid,region,body,withInlineData,withoutInlineData,
            badHexChunks,inlineTextChars,inlineBinaryBytes);
    }

    public static final class Snapshot{
        public final int solidCount,regionCount,bodyCount;
        public final int withInlineData,withoutInlineData,invalidHexChunks;
        public final long inlineTextChars,inlineBinaryBytes;
        private Snapshot(int solid,int region,int body,int withData,int missing,int invalid,
                         long ascii,long binary){
            solidCount=solid;regionCount=region;bodyCount=body;withInlineData=withData;
            withoutInlineData=missing;invalidHexChunks=invalid;
            inlineTextChars=ascii;inlineBinaryBytes=binary;
        }
        public int entityCount(){return solidCount+regionCount+bodyCount;}
        public boolean hasMissingBodies(){return withoutInlineData>0;}
        /** A record with a payload is NOT proof that a full 3D B-rep was decoded. */
        public boolean fullyRendered(){return entityCount()==0;}
    }
}
