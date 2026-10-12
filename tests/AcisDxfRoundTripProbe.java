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
        int bodies,meshBodies,completeBodies,partialBodies,faces,triangles,edges;
        void finish(String type,List<String> chunks,boolean publicFixtureDebug){
            if(!"3DSOLID".equals(type)&&!"REGION".equals(type)&&!"BODY".equals(type))return;
            bodies++;
            AcisSatMesh.Result mesh=AcisSatMesh.parseDxfChunks(chunks);
            if(publicFixtureDebug){
                StringBuilder raw=new StringBuilder();
                for(String chunk:chunks){if(raw.length()>0)raw.append(' ');raw.append(chunk);}
                String decoded=AcisSatMeshDebug.decode(raw.toString().replace("^ ","^"));
                java.util.TreeMap<String,Integer> types=new java.util.TreeMap<>();
                for(String rec:decoded.split("#")){
                    int ordinal=recordOrdinal++;
                    String[] words=rec.trim().split("\\s+");
                    if(words.length<2)continue;
                    String kind=words[0].matches("-\\d+")?words[1]:words[0];
                    if(kind.endsWith("-surface")||java.util.Arrays.asList("body","lump","shell","face","loop","coedge","edge","vertex","point","transform","asmheader").contains(kind))
                        types.merge(kind,1,Integer::sum);
                }
                java.util.Map<String,Integer> sampleCounts=new java.util.HashMap<>();
                int recordOrdinal=0;
                for(String rec:decoded.split("#")){
                    String[] words=rec.trim().split("\\s+");
                    if(words.length<2)continue;
                    String kind=words[0].matches("-\\d+")?words[1]:words[0];
                    if(!java.util.Arrays.asList("face","loop","coedge","edge","vertex","point","body","shell").contains(kind))continue;
                    int index=sampleCounts.getOrDefault(kind,0);sampleCounts.put(kind,index+1);
                    if(index<2&&(bodies==1||bodies==3))
                        System.out.printf("ACIS_PUBLIC_REF ordinal=%d kind=%s record=%s%n",ordinal,kind,
                            rec.trim().substring(0,Math.min(220,rec.trim().length())).replaceAll("[^a-zA-Z0-9. $-]","_"));
                }

                String prefix=decoded.substring(0,Math.min(110,decoded.length())).replaceAll("[^a-zA-Z0-9. -]","_");
                System.out.printf("ACIS_PUBLIC_FIXTURE_BODY index=%d kind=%s chunks=%d chars=%d classes=%s head=%s%n",
                    bodies,type,chunks.size(),decoded.length(),types,prefix);
            }
            if(mesh.hasGeometry())meshBodies++;
            if(mesh.complete)completeBodies++;
            if(mesh.hasGeometry()&&!mesh.complete)partialBodies++;
            faces+=mesh.faces;
            triangles+=mesh.triangles.length/3;
            edges+=mesh.edges.length/2;
        }
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
        System.out.printf("ACIS_REAL_DXF_MESH bodies=%d mesh=%d complete=%d partial=%d faces=%d triangles=%d edges=%d%n",
                counts.bodies,counts.meshBodies,counts.completeBodies,counts.partialBodies,
                counts.faces,counts.triangles,counts.edges);
        if(counts.bodies==0)throw new AssertionError("Real test did not contain ACIS entities");
        // This probe is diagnostic until a confirmed compatible source drawing
        // and closed B-Rep expected-triangle contract are established.
    }
}
