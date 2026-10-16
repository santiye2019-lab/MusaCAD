package com.musa.cad;

import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Portable-JVM, read-only cross-check for original DWG top-level entity handles
 * versus DXF group-5 candidates. No gesture activation or edit permission.
 *
 * A positive result means matching handle + known top-level entity type ONLY.
 * It does not prove primitive-level instance ancestry, geometry equality,
 * provenance of the converter, or AutoCAD 2026 behaviour.
 */
public final class Cad3dDwgHandleCrosscheck {
    public static final int MAX_MODEL_ENTITIES=250_000;
    public static final int MAX_PAGE_ENTRIES=1024;
    public static final int TYPE_OTHER=0,TYPE_LINE=1,TYPE_3DFACE=2,
                            TYPE_TRACE=3,TYPE_SOLID=4;

    private final Map<Long,Integer> directModels;
    private Cad3dDwgHandleCrosscheck(Map<Long,Integer> checked) {
        directModels=Collections.unmodifiableMap(new HashMap<>(checked));
    }

    public int modelEntityCount(){return directModels.size();}

    public boolean matchesDirectModel(Cad3dDxfHandleIndex.Candidate candidate) {
        if(candidate==null)return false;
        int expected=typeOf(candidate.entityType);
        if(expected==TYPE_OTHER)return false;
        // Parse all 64 handle bits without requiring Android API 26's
        // Long.parseUnsignedLong; every nibble is bounded and lossless.
        String hex=candidate.dxfHandle;
        if(hex==null||hex.isEmpty()||hex.length()>16)return false;
        long key=0;
        for(int i=0;i<hex.length();i++){
            char c=hex.charAt(i);
            int digit=c>='0'&&c<='9'?c-'0'
                :c>='A'&&c<='F'?c-'A'+10
                :c>='a'&&c<='f'?c-'a'+10:-1;
            if(digit<0)return false;
            key=(key<<4)|(long)digit;
        }
        if(key==0)return false;
        Integer nativeType=directModels.get(key);
        return nativeType!=null&&nativeType==expected;
    }

    /** Model owner only. Nested INSERTs require handle + instance path. */
    private static int typeOf(String type){
        if("LINE".equals(type))return TYPE_LINE;
        if("3DFACE".equals(type))return TYPE_3DFACE;
        if("TRACE".equals(type))return TYPE_TRACE;
        if("SOLID".equals(type))return TYPE_SOLID;
        return TYPE_OTHER;
    }

    public static final class Builder {
        private final Map<Long,Integer> records=new HashMap<>();
        private long nextTop;
        private boolean finished;
        private boolean failed;

        /**
         * The original session's exact top-level cursor must advance by the
         * number of entities. Reject overlapping, missing, duplicate, zero,
         * mis-typed or oversized records; do not accept partially read data.
         */
        public void addPage(long first,long[] packed,long next,boolean hasMore)
                throws IOException {
            if(finished||failed)throw new IOException("DWG kimlik sayfalaması kapalı");
            try{
                if(first!=nextTop||packed==null||packed.length<2||
                   (packed.length&1)!=0||packed.length>2*MAX_PAGE_ENTRIES||
                   next!=first+packed.length/2L||next>MAX_MODEL_ENTITIES||
                   (hasMore&&next==MAX_MODEL_ENTITIES))
                    throw new IOException("DWG kimlik devam imleci veya sınırı geçersiz");
                for(int i=0;i<packed.length;i+=2){
                    long handle=packed[i],type=packed[i+1];
                    if(handle==0||type<TYPE_OTHER||type>TYPE_SOLID)
                        throw new IOException("DWG nesne kimliği/türü geçersiz");
                    if(records.putIfAbsent(handle,(int)type)!=null)
                        throw new IOException("DWG nesne kimliği yineleniyor");
                }
                nextTop=next;
                if(!hasMore)finished=true;
            }catch(IOException failure){
                failed=true;
                records.clear();
                throw failure;
            }
        }

        public Cad3dDwgHandleCrosscheck finish() throws IOException {
            if(!finished||failed||records.isEmpty())
                throw new IOException("DWG nesne kimliği sayfaları tamamlanmadı");
            failed=true;
            return new Cad3dDwgHandleCrosscheck(records);
        }
    }
}
