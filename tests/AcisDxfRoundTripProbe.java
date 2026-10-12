import com.musa.cad.AcisSatMesh;
import java.io.BufferedReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * Read-only real-world DWG->DXF SAT-to-mesh diagnostic.
 * Geometry assertions must only be enabled for independently known fixtures.
 * This prints coverage; it never invents triangles for unsupported surfaces.
 */
public final class AcisDxfRoundTripProbe {
    private static final class Stats {
        int bodies,meshBodies,completeBodies,partialBodies,faces,tessellatedFaces,unsupportedFaces,triangles,edges,danglingReferences;
        void finish(String type,List<String> chunks,boolean publicFixtureDebug)throws Exception{
            if(!"3DSOLID".equals(type)&&!"REGION".equals(type)&&!"BODY".equals(type))return;
            bodies++;
            AcisSatMesh.Result mesh=AcisSatMesh.parseDxfChunks(chunks);
            if(publicFixtureDebug){
                StringBuilder raw=new StringBuilder();
                for(String chunk:chunks){if(raw.length()>0)raw.append(' ');raw.append(chunk);}
                String decoded=AcisSatMeshDebug.decode(raw.toString().replace("^ ","^"));
                if(bodies==2||bodies==3){
                    java.util.ArrayList<Integer> lens=new java.util.ArrayList<>();
                    for(String part:chunks)lens.add(part.length());
                    int at=0;
                    for(int i=0;i<chunks.size();i++){
                        if(chunks.get(i).length()>=250){
                            String tail=chunks.get(i).substring(Math.max(0,chunks.get(i).length()-14));
                            String next=i+1<chunks.size()?chunks.get(i+1).substring(0,Math.min(14,chunks.get(i+1).length())):"";
                            System.out.printf("ACIS_PUBLIC_CHUNK_SEAM body=%d idx=%d len=%d tail=%s next=%s%n",
                                bodies,i,chunks.get(i).length(),tail.replaceAll("[^A-Za-z0-9 ]","_"),
                                next.replaceAll("[^A-Za-z0-9 ]","_"));
                        }
                        at+=chunks.get(i).length();
                    }
                    System.out.printf("ACIS_PUBLIC_CHUNK_LENGTHS body=%d lengths=%s%n",bodies,lens);
                }

                java.util.TreeMap<String,Integer> types=new java.util.TreeMap<>();
                for(String rec:decoded.split("#")){
                    String[] words=rec.trim().split("\\s+");
                    if(words.length<2)continue;
                    String kind=words[0].matches("-\\d+")?words[1]:words[0];
                    if(kind.endsWith("-surface")||java.util.Arrays.asList("body","lump","shell","face","loop","coedge","edge","vertex","point","transform","asmheader").contains(kind))
                        types.merge(kind,1,Integer::sum);
                    else if(bodies==3&&kind.length()<14&&!"straight-curve".equals(kind)&&
                            !"ellipse-curve".equals(kind)&&"strai".equals(kind))
                        System.out.printf("ACIS_PUBLIC_ODD_RECORD body=%d kind=%s text=%s%n",bodies,kind,
                            rec.trim().substring(0,Math.min(90,rec.trim().length())).replaceAll("[^a-zA-Z0-9. $-]","_"));
                }
                java.util.Map<String,Integer> sampleCounts=new java.util.HashMap<>();
                int recordOrdinal=0;
                for(String rec:decoded.split("#")){
                    int ordinal=recordOrdinal++;
                    String[] words=rec.trim().split("\\s+");
                    if(words.length<2)continue;
                    String kind=words[0].matches("-\\d+")?words[1]:words[0];
                    if(!java.util.Arrays.asList("face","loop","coedge","edge","vertex","point","body","shell").contains(kind))continue;
                    int index=sampleCounts.getOrDefault(kind,0);sampleCounts.put(kind,index+1);
                    if(index<2&&(bodies>=1&&bodies<=3))
                        System.out.printf("ACIS_PUBLIC_REF ordinal=%d kind=%s record=%s%n",ordinal,kind,
                            rec.trim().substring(0,Math.min(220,rec.trim().length())).replaceAll("[^a-zA-Z0-9. $-]","_"));
                }

                String prefix=decoded.substring(0,Math.min(110,decoded.length())).replaceAll("[^a-zA-Z0-9. -]","_");
                System.out.printf("ACIS_PUBLIC_FIXTURE_BODY index=%d kind=%s chunks=%d chars=%d classes=%s head=%s%n",
                    bodies,type,chunks.size(),decoded.length(),types,prefix);
                if(bodies>=1&&bodies<=3)debugParseTopology(decoded,bodies);
            }
            if(mesh.hasGeometry())meshBodies++;
            if(mesh.complete)completeBodies++;
            if(mesh.hasGeometry()&&!mesh.complete)partialBodies++;
            faces+=mesh.faces;
            tessellatedFaces+=mesh.tessellatedFaces;
            unsupportedFaces+=mesh.unsupportedFaces;
            danglingReferences+=mesh.danglingReferences;
            triangles+=mesh.triangles.length/3;
            edges+=mesh.edges.length/2;
        }
    }
    /** Read-only access to production parser internals for PUBLIC pinned fixture diagnostics. */
    private static void debugParseTopology(String decoded,int body)throws Exception{
        Class<?> nodeClass=Class.forName("com.musa.cad.AcisSatMesh$Node");
        java.lang.reflect.Method parse=AcisSatMesh.class.getDeclaredMethod("parseNodes",String.class);
        parse.setAccessible(true);
        @SuppressWarnings("unchecked")
        java.util.Map<Integer,?> nodes=(java.util.Map<Integer,?>)parse.invoke(null,decoded);
        int satRecordCount=0;
        int malformedRecordTypes=0;
        for(String record:decoded.split("#")){
            String[] fields=record.trim().split("\\s+");
            if(fields.length<2||record.trim().startsWith("End-of-ACIS"))continue;
            satRecordCount++;
            // First record carries an ASCII SAT header followed by asmheader.
            String typeName=fields[0].matches("-\\d+")?fields[1]:fields[0];
            if(typeName.length()<2)malformedRecordTypes++;
        }
        System.out.printf("ACIS_PUBLIC_RECORD_PARITY body=%d source_records=%d parsed_nodes=%d excess_records=%d%n",
            body,satRecordCount,nodes.size(),satRecordCount-nodes.size());
        if(satRecordCount!=nodes.size())
            throw new AssertionError("Public R2013 SAT records silently lost by MusaCAD parser");

        java.lang.reflect.Field type=nodeClass.getDeclaredField("type");
        java.lang.reflect.Field id=nodeClass.getDeclaredField("id");
        java.lang.reflect.Field refs=nodeClass.getDeclaredField("refs");
        type.setAccessible(true);id.setAccessible(true);refs.setAccessible(true);
        java.lang.reflect.Method sample=AcisSatMesh.class.getDeclaredMethod("sampleEdge",java.util.Map.class,nodeClass,boolean.class);
        sample.setAccessible(true);
        java.lang.reflect.Method build=AcisSatMesh.class.getDeclaredMethod("buildLoop",java.util.Map.class,nodeClass);
        build.setAccessible(true);
        // Public R2013 fixture: establish which two source faces fail, and
        // distinguish missing loop topology from unsupported surfaces. Only
        // record IDs, surface classes and vertex COUNTS are logged.
        java.lang.reflect.Method loopsForFace=AcisSatMesh.class.getDeclaredMethod(
            "loopsForFace",java.util.Map.class,int.class);
        loopsForFace.setAccessible(true);
        java.lang.reflect.Method surfaceOfFace=AcisSatMesh.class.getDeclaredMethod(
            "firstRefBySuffix",nodeClass,java.util.Map.class,String.class);
        surfaceOfFace.setAccessible(true);
        for(Object faceNode:nodes.values()){
            if(!"face".equals(type.get(faceNode)))continue;
            int faceId=(Integer)id.get(faceNode);
            @SuppressWarnings("unchecked")
            java.util.List<?> loops=(java.util.List<?>)loopsForFace.invoke(null,nodes,faceId);
            Object surface=surfaceOfFace.invoke(null,faceNode,nodes,"-surface");
            String surfaceName=surface==null?"UNRESOLVED":(String)type.get(surface);
            java.util.ArrayList<String> paths=new java.util.ArrayList<>();
            for(Object loop:loops){
                Object path=build.invoke(null,nodes,loop);
                java.lang.reflect.Field points=path.getClass().getDeclaredField("points");
                java.lang.reflect.Field exact=path.getClass().getDeclaredField("exact");
                points.setAccessible(true);exact.setAccessible(true);
                paths.add(id.get(loop)+":"+((java.util.List<?>)points.get(path)).size()+
                    ":"+(Boolean)exact.get(path));
            }
            System.out.printf("ACIS_PUBLIC_FACE body=%d id=%d surface=%s loop_count=%d loop_paths=%s%n",
                body,faceId,surfaceName,loops.size(),paths);
        }

        int pointsInLoops=0,nonEmptyLoops=0,nonEmptyEdges=0;
        int inspected=0;
        java.util.ArrayList<Integer> sampleIds=new java.util.ArrayList<>();
        for(Object n:nodes.values()){
            String kind=(String)type.get(n);
            if(("edge".equals(kind)||"vertex".equals(kind)||"face".equals(kind))&&inspected<12){
                int[] links=(int[])refs.get(n);
                StringBuilder resolved=new StringBuilder();
                for(int i=0;i<links.length&&i<9;i++){
                    Object target=nodes.get(links[i]);
                    resolved.append(links[i]).append('=')
                        .append(target==null?"missing":type.get(target)).append(' ');
                }
                System.out.printf("ACIS_PUBLIC_LINK body=%d id=%d kind=%s refs=%s%n",
                    body,id.get(n),kind,resolved);
                inspected++;
            }
            if("loop".equals(kind)){
                Object path=build.invoke(null,nodes,n);
                java.lang.reflect.Field points=path.getClass().getDeclaredField("points");
                points.setAccessible(true);
                int count=((java.util.List<?>)points.get(path)).size();
                pointsInLoops+=count;
                if(count>0)nonEmptyLoops++;
                if(sampleIds.size()<3)sampleIds.add((Integer)id.get(n));
            }else if("edge".equals(kind)){
                Object path=sample.invoke(null,nodes,n,false);
                java.lang.reflect.Field points=path.getClass().getDeclaredField("points");
                points.setAccessible(true);
                if(((java.util.List<?>)points.get(path)).size()>=2)nonEmptyEdges++;
            }
        }
        if(body==2||body==3){
            // Pinned PUBLIC test-data/example_2013.dwg only, never customer content.
            int emitted=0;
            for(Object n:nodes.values()){
                int number=(Integer)id.get(n);
                if(body==2||number<46||number>=110){
                    int[] links=(int[])refs.get(n);
                    StringBuilder mapped=new StringBuilder();
                    for(int i=0;i<Math.min(links.length,8);i++){
                        Object dest=nodes.get(links[i]);
                        mapped.append(links[i]).append(':')
                            .append(dest==null?"MISSING":type.get(dest)).append(',');
                    }
                    System.out.printf("ACIS_PUBLIC_GRAPH body=%d id=%d kind=%s refs=%s%n",body,
                         number,type.get(n),mapped);
                    if(++emitted>=115)break;
                }
            }
        }
        System.out.printf("ACIS_PUBLIC_INTERNAL body=%d nodes=%d loopIds=%s liveLoops=%d loopPoints=%d liveEdges=%d%n",
            body,nodes.size(),sampleIds,nonEmptyLoops,pointsInLoops,nonEmptyEdges);
    }

    private static final class AcisSatMeshDebug{
        static String decode(String data){
            StringBuilder out=new StringBuilder(data.length());
            for(int i=0;i<data.length();i++){
                char c=data.charAt(i);out.append(c>32&&c<159?(char)(159-c):c);
            }
            return out.toString();
        }
    }
    public static void main(String[] args)throws Exception{
        if(args.length<1||args.length>2||args.length==2&&!"--public-fixture-debug".equals(args[1]))
            throw new IllegalArgumentException("Usage: Probe exported.dxf [--public-fixture-debug]");
        boolean publicFixtureDebug=args.length==2;
        Stats counts=new Stats();
        try(BufferedReader reader=Files.newBufferedReader(Path.of(args[0]),StandardCharsets.ISO_8859_1)){
            String section="",type="",codeLine,valueLine;
            boolean needSection=false;
            List<String> chunks=new ArrayList<>();
            while((codeLine=reader.readLine())!=null){
                valueLine=reader.readLine();
                if(valueLine==null)throw new IllegalStateException("Incomplete DXF pair");
                int code=Integer.parseInt(codeLine.trim());
                String value=valueLine.strip();
                if(needSection){
                    if(code!=2)throw new IllegalStateException("Missing section");
                    section=value.toUpperCase();needSection=false;continue;
                }
                if(code==0){
                    counts.finish(type,chunks,publicFixtureDebug);
                    chunks.clear();type="";
                    if("SECTION".equalsIgnoreCase(value))needSection=true;
                    else if("ENDSEC".equalsIgnoreCase(value))section="";
                    else if(("ENTITIES".equals(section)||"BLOCKS".equals(section))&&
                            ("3DSOLID".equalsIgnoreCase(value)||"REGION".equalsIgnoreCase(value)||"BODY".equalsIgnoreCase(value)))
                        type=value.toUpperCase();
                }else if(!type.isEmpty()&&(code==1||code==3)){
                    chunks.add(value);
                }
            }
            counts.finish(type,chunks,publicFixtureDebug);
        }
        System.out.printf("ACIS_REAL_DXF_MESH bodies=%d mesh=%d complete=%d partial=%d faces=%d tessellated=%d unsupported=%d triangles=%d edges=%d dangling=%d%n",
                counts.bodies,counts.meshBodies,counts.completeBodies,counts.partialBodies,
                counts.faces,counts.tessellatedFaces,counts.unsupportedFaces,counts.triangles,counts.edges,counts.danglingReferences);
        if(counts.bodies==0)throw new AssertionError("Real test did not contain ACIS entities");
        if(publicFixtureDebug){
            // This is ONLY used with the pinned public LibreDWG example_2013.dwg.
            // We require one independently observed planar REGION to remain
            // triangulated, but do not claim that the other two B-Reps work.
            if(counts.bodies!=3||counts.faces!=10||counts.completeBodies<1||counts.triangles<2||
               counts.tessellatedFaces+counts.unsupportedFaces!=counts.faces||counts.danglingReferences==0)
                throw new AssertionError("R2013 ACIS planar face regression: expected 3 source bodies and at least one closed, triangulated face");
            System.out.println("PASS: pinned public R2013 has a real triangulated ACIS face; unresolved bodies remain explicitly incomplete");
        }

        // This probe is diagnostic until a confirmed compatible source drawing
        // and closed B-Rep expected-triangle contract are established.
    }
}
