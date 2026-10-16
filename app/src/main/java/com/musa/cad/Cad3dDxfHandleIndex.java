package com.musa.cad;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * READ-ONLY identity candidates for the existing DWG-to-DXF 3D view.
 *
 * A DXF group-5 handle is NOT independently verified against the original
 * binary DWG. In particular, an expanded INSERT can reuse one block entity
 * handle for many placements. Never substitute triangle/vertex indices or
 * source line numbers for an original DWG entity ID.
 *
 * This index only exposes direct-root LINE, 3DFACE, TRACE and SOLID records
 * for which Dxf3dMesh retained an exact source-line association. Tessellated
 * curves, ACIS proxies, expanded blocks and polyface vertices fail closed.
 * No user gesture, save or entity mutation is enabled by this diagnostic.
 */
public final class Cad3dDxfHandleIndex {
    private static final int MAX_INDEXED_HANDLES=300_000;
    private final CadSaveSourceIntegrity sourceIntegrity;
    private final Map<Integer,Candidate> bySourceStart;

    /** A candidate from the DXF intermediate, never a verified native DWG ID. */
    public static final class Candidate {
        public final String dxfHandle,entityType;
        public final int sourceStartLine;
        private Candidate(String handle,String type,int start) {
            this.dxfHandle=handle;this.entityType=type;this.sourceStartLine=start;
        }
    }
    private Cad3dDxfHandleIndex(CadSaveSourceIntegrity integrity,
            Map<Integer,Candidate> indexed) {
        sourceIntegrity=integrity;
        bySourceStart=Collections.unmodifiableMap(new HashMap<>(indexed));
    }

    /**
     * Runs on the CAD load worker, NEVER during Android drawing or touch.
     * SHA-256 binds the index to the exact intermediate DXF byte contents.
     * Missing handles remain unavailable; malformed or reused nonzero handles
     * fail the whole index rather than fabricating stable identity.
     */
    public static Cad3dDxfHandleIndex read(File source) throws IOException {
        CadSaveSourceIntegrity before=CadSaveSourceIntegrity.capture(source);
        Map<Integer,Candidate> entries=new HashMap<>();
        Set<String> seen=new HashSet<>();
        String section="";
        try(DxfStream stream=new DxfStream(source,StandardCharsets.UTF_8)) {
            DxfBlocks.Record r;
            while((r=stream.next())!=null) {
                if("SECTION".equals(r.type)) {
                    section=r.text(2,"");
                    continue;
                }
                if("ENDSEC".equals(r.type)) {section="";continue;}
                if(!"ENTITIES".equalsIgnoreCase(section))continue;
                String raw=null;
                for(int k=r.from;k+1<r.to;k+=2) {
                    if(Integer.parseInt(r.tags().get(k))!=5)continue;
                    if(raw!=null)throw new IOException("Yinelenen DXF grup-5 kimliği");
                    raw=r.tags().get(k+1).trim();
                }
                if(raw==null)continue;
                String handle=canonicalHandle(raw);
                System.out.println("DXF fixture identity trace type="+r.type+" start="+r.sourceStart+" handle="+handle);
                if(!seen.add(handle))
                    throw new IOException("Farklı DXF nesneleri aynı tutamacı kullanıyor: "+handle+" type="+r.type+" at="+r.sourceStart);
                if(seen.size()>MAX_INDEXED_HANDLES)
                    throw new IOException("DXF kimlik dizini kapasite sınırı");
                if(supported(r.type)) {
                    int start=r.sourceStart;
                    if(start<0||entries.put(start,
                            new Candidate(handle,r.type,start))!=null)
                        throw new IOException("DXF kaynak konumu belirsiz");
                }
            }
        }
        before.requireUnchanged(source);
        return new Cad3dDxfHandleIndex(before,entries);
    }

    /** Required before using candidates for any subsequent source operation. */
    public void requireUnchanged(File source) throws IOException {
        sourceIntegrity.requireUnchanged(source);
    }

    public int indexedRecordCount() {return bySourceStart.size();}

    public Candidate forVertex(Dxf3dMesh mesh,int vertex) {
        if(mesh==null||vertex<0||vertex>=mesh.sourceLines.length||
           mesh.xyz.length/3!=mesh.sourceLines.length)return null;
        int start=mesh.sourceLines[vertex];
        return start<0?null:bySourceStart.get(start);
    }

    /** Both authored edge vertices MUST belong to the same source entity. */
    public Candidate forEdge(Dxf3dMesh mesh,int edge) {
        if(mesh==null||edge<0||edge>=mesh.edges.length/2)return null;
        int a=mesh.edges[edge*2],b=mesh.edges[edge*2+1];
        Candidate ca=forVertex(mesh,a),cb=forVertex(mesh,b);
        return sameRecord(ca,cb)?ca:null;
    }

    /** All three triangle vertices MUST map to one original direct-root face. */
    public Candidate forTriangle(Dxf3dMesh mesh,int triangle) {
        if(mesh==null||triangle<0||triangle>=mesh.triangles.length/3)return null;
        int at=triangle*3;
        Candidate a=forVertex(mesh,mesh.triangles[at]);
        Candidate b=forVertex(mesh,mesh.triangles[at+1]);
        Candidate c=forVertex(mesh,mesh.triangles[at+2]);
        if(a==null||!"3DFACE".equals(a.entityType)&&
           !"TRACE".equals(a.entityType)&&!"SOLID".equals(a.entityType))
            return null;
        return sameRecord(a,b)&&sameRecord(a,c)?a:null;
    }

    private static boolean sameRecord(Candidate a,Candidate b) {
        return a!=null&&a==b;
    }
    private static boolean supported(String type) {
        return "LINE".equals(type)||"3DFACE".equals(type)||
            "TRACE".equals(type)||"SOLID".equals(type);
    }
    private static String canonicalHandle(String raw) throws IOException {
        if(raw.isEmpty()||raw.length()>16)
            throw new IOException("DXF grup-5 kimlik uzunluğu geçersiz");
        for(int k=0;k<raw.length();k++)
            if(Character.digit(raw.charAt(k),16)<0||
               raw.charAt(k)>127)
                throw new IOException("DXF grup-5 hex kimliği geçersiz");
        int first=0;
        while(first<raw.length()-1&&raw.charAt(first)=='0')first++;
        String handle=raw.substring(first).toUpperCase(Locale.ROOT);
        if("0".equals(handle))
            throw new IOException("Sıfır DXF nesne tutamacı geçersiz");
        return handle;
    }
}
