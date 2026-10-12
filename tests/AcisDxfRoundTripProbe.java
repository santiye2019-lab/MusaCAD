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
        void finish(String type,List<String> chunks){
            if(!"3DSOLID".equals(type)&&!"REGION".equals(type)&&!"BODY".equals(type))return;
            bodies++;
            AcisSatMesh.Result mesh=AcisSatMesh.parseDxfChunks(chunks);
            if(mesh.hasGeometry())meshBodies++;
            if(mesh.complete)completeBodies++;
            if(mesh.hasGeometry()&&!mesh.complete)partialBodies++;
            faces+=mesh.faces;
            triangles+=mesh.triangles.length/3;
            edges+=mesh.edges.length/2;
        }
    }
    public static void main(String[] args)throws Exception{
        if(args.length!=1)throw new IllegalArgumentException("Usage: Probe exported.dxf");
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
                    counts.finish(type,chunks);
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
            counts.finish(type,chunks);
        }
        System.out.printf("ACIS_REAL_DXF_MESH bodies=%d mesh=%d complete=%d partial=%d faces=%d triangles=%d edges=%d%n",
                counts.bodies,counts.meshBodies,counts.completeBodies,counts.partialBodies,
                counts.faces,counts.triangles,counts.edges);
        if(counts.bodies==0)throw new AssertionError("Real test did not contain ACIS entities");
        // This probe is diagnostic until a confirmed compatible source drawing
        // and closed B-Rep expected-triangle contract are established.
    }
}
