import com.musa.cad.DxfBlocks;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

/** Autodesk OCS arbitrary-axis basis parity for nested, tilted and MINSERT blocks. */
public final class DxfTiltedOcsTest {
    private static String tags(Object...values){
        StringBuilder b=new StringBuilder();
        for(Object x:values)b.append(x).append('\n');
        return b.toString();
    }
    private static String block(String name,String body){
        return tags(0,"BLOCK",2,name,10,0,20,0,30,0)+body+tags(0,"ENDBLK");
    }
    private static String line(){
        return tags(0,"LINE",10,0,20,0,30,0,11,2,21,0,31,0);
    }
    private static String document(String blocks,String entities){
        return tags(0,"SECTION",2,"BLOCKS")+blocks+tags(0,"ENDSEC",0,"SECTION",2,"ENTITIES")
            +entities+tags(0,"ENDSEC",0,"EOF");
    }
    private static void eq(double got,double expected,String field){
        if(Math.abs(got-expected)>1e-4)
            throw new AssertionError(field+": "+got+" != "+expected);
    }
    private static void xyz(Dxf3dMesh model,int vertex,double x,double y,double z){
        int k=vertex*3;
        eq(model.xyz[k],x,"vertex "+vertex+" X");
        eq(model.xyz[k+1],y,"vertex "+vertex+" Y");
        eq(model.xyz[k+2],z,"vertex "+vertex+" Z");
    }
    private static Dxf3dMesh check(String dxf,int placements)throws Exception{
        DxfBlocks.Result expanded=DxfBlocks.expand(Arrays.asList(dxf.split("\n")));
        if(expanded.skipped!=0||expanded.placements.size()!=placements)
            throw new AssertionError("OCS object expansion "+expanded.placements.size()+" skipped "+expanded.skipped);
        File file=File.createTempFile("musacad-ocs-", ".dxf");
        try{
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh read=Dxf3dMesh.read(file);
            if(read.xyz.length!=placements*6||read.edges.length!=placements*2)
                throw new AssertionError("OCS 3D mesh geometry unexpected");
            for(int line:read.sourceLines)if(line!=-1)
                throw new AssertionError("expanded OCS block source must be read-only");
            return read;
        }finally{Files.deleteIfExists(file.toPath());}
    }
    private static void basicTiltedNormals()throws Exception{
        String b=block("PIPE",line());
        Dxf3dMesh x=check(document(b,tags(0,"INSERT",2,"PIPE",10,5,20,6,30,7,210,1,220,0,230,0)),1);
        // OCS normal X: basis X=(0,1,0), Y=(0,0,1), Z=(1,0,0).
        xyz(x,0,7,5,6);xyz(x,1,7,7,6);
        Dxf3dMesh y=check(document(b,tags(0,"INSERT",2,"PIPE",10,5,20,6,30,7,210,0,220,1,230,0)),1);
        // OCS normal Y: basis X=(-1,0,0), Y=(0,0,1), Z=(0,1,0).
        xyz(y,0,-5,7,6);xyz(y,1,-7,7,6);
        Dxf3dMesh slope=check(document(b,tags(0,"INSERT",2,"PIPE",10,2,20,3,30,4,210,0,220,1,230,1)),1);
        double root2=Math.sqrt(2d);
        xyz(slope,0,-2,1/root2,7/root2);
        xyz(slope,1,-4,1/root2,7/root2);
        Dxf3dMesh lengthNormal=check(document(b,tags(0,"INSERT",2,"PIPE",10,5,20,6,30,7,210,10,220,0,230,0)),1);
        xyz(lengthNormal,0,7,5,6);xyz(lengthNormal,1,7,7,6);
    }
    private static void rotatedScaledInclinedInsert()throws Exception{
        String b=block("PIPE",line());
        // OCS normal X, 90deg rotation sends local X to OCS Y (world Z).
        Dxf3dMesh m=check(document(b,tags(0,"INSERT",2,"PIPE",10,5,20,6,30,7,
            210,1,220,0,230,0,50,90,41,2,42,3,43,4)),1);
        xyz(m,0,7,5,6);xyz(m,1,7,5,10);
    }
    private static void nestedTiltsCompose()throws Exception{
        String blocks=block("INNER",tags(0,"LINE",10,0,20,0,30,0,11,1,21,0,31,0))
            +block("OUTER",tags(0,"INSERT",2,"INNER",10,1,20,2,30,3,210,1,220,0,230,0));
        String roots=tags(0,"INSERT",2,"OUTER",10,4,20,5,30,6,210,0,220,1,230,0);
        Dxf3dMesh m=check(document(blocks,roots),1);
        xyz(m,0,-7,8,6);xyz(m,1,-7,8,7);
    }
    private static void mInsertArrayInTiltedPlane()throws Exception{
        String blocks=block("PIPE",tags(0,"LINE",10,0,20,0,30,0,11,1,21,0,31,0));
        String roots=tags(0,"INSERT",2,"PIPE",10,1,20,2,30,3,210,1,220,0,230,0,
            70,2,71,2,44,5,45,7);
        Dxf3dMesh m=check(document(blocks,roots),4);
        xyz(m,0,3,1,2);xyz(m,2,3,6,2);
        xyz(m,4,3,1,9);xyz(m,6,3,6,9);
    }
    private static void sourceZCrossAxisAndBlockBase()throws Exception{
        // A tilted extrusion must project each source Z into WCS X, not discard it.
        String b=tags(0,"BLOCK",2,"PIPE_Z",10,0,20,0,30,1)
            +tags(0,"LINE",10,0,20,0,30,2,11,0,21,0,31,5)
            +tags(0,"ENDBLK");
        String insert=tags(0,"INSERT",2,"PIPE_Z",10,1,20,2,30,3,
            210,1,220,0,230,0);
        Dxf3dMesh m=check(document(b,insert),1);
        // Base Z=1 is removed before the OCS axis is applied.
        xyz(m,0,4,1,2);
        xyz(m,1,7,1,2);
    }
    private static void tiltedAcadTableAndDimension()throws Exception{
        // ACAD_TABLE heading group 11 points along world Z, with extrusion along world X.
        String tableBlock=block("*TABLE",tags(0,"LINE",10,0,20,0,30,0,11,2,21,0,31,0));
        String table=tags(0,"ACAD_TABLE",2,"*TABLE",10,5,20,6,30,7,
            11,0,21,0,31,1,210,1,220,0,230,0);
        Dxf3dMesh tableMesh=check(document(tableBlock,table),1);
        xyz(tableMesh,0,7,5,6);xyz(tableMesh,1,7,5,8);

        // Dimension clone group-12 origin is OCS, not ordinary WCS translation.
        String dimensionBlock=block("*D1",tags(0,"LINE",10,0,20,0,30,0,11,2,21,0,31,0));
        String dimension=tags(0,"DIMENSION",2,"*D1",12,5,22,6,32,7,
            210,0,220,1,230,0,70,32);
        Dxf3dMesh dimMesh=check(document(dimensionBlock,dimension),1);
        xyz(dimMesh,0,-5,7,6);xyz(dimMesh,1,-7,7,6);
    }
    private static void nestedTiltedTable()throws Exception{
        String blocks=block("*TABLE",tags(0,"LINE",10,0,20,0,30,0,11,2,21,0,31,0))
            +block("OUTER",tags(0,"ACAD_TABLE",2,"*TABLE",
                10,5,20,6,30,7,11,0,21,0,31,1,210,1,220,0,230,0));
        Dxf3dMesh m=check(document(blocks,
            tags(0,"INSERT",2,"OUTER",10,1,20,2,30,3,210,0,220,1,230,0)),1);
        // Inner TABLE starts at (7,5,6), ends at (7,5,8).
        // Outer OCS with normal Y: WCS X=(-1,0,0), Y=(0,0,1), Z=(0,1,0).
        xyz(m,0,-6,9,7);xyz(m,1,-6,11,7);
    }
    private static void invalidHeadingDoesNotFabricateTable()throws Exception{
        String b=block("*TABLE",line());
        String invalid=tags(0,"ACAD_TABLE",2,"*TABLE",11,1,21,0,31,0,
            210,1,220,0,230,0);
        String dxf=document(b,invalid);
        DxfBlocks.Result expanded=DxfBlocks.expand(Arrays.asList(dxf.split("\n")));
        if(expanded.skipped!=1||!expanded.placements.isEmpty())
            throw new AssertionError("table heading parallel to extrusion must not invent a rotation");
    }
    private static void zeroExtrusionIsSkipped()throws Exception{
        String dxf=document(block("PIPE",line()),
            tags(0,"INSERT",2,"PIPE",210,0,220,0,230,0));
        DxfBlocks.Result result=DxfBlocks.expand(Arrays.asList(dxf.split("\n")));
        if(result.skipped!=1||!result.placements.isEmpty())
            throw new AssertionError("zero OCS extrusion must not silently place a block");
    }
    public static void main(String[] args)throws Exception{
        basicTiltedNormals();
        rotatedScaledInclinedInsert();
        nestedTiltsCompose();
        sourceZCrossAxisAndBlockBase();
        mInsertArrayInTiltedPlane();
        zeroExtrusionIsSkipped();
        tiltedAcadTableAndDimension();
        nestedTiltedTable();
        invalidHeadingDoesNotFabricateTable();
        System.out.println("3D OCS: tilted X/Y/diagonal, nonunit normals, 90deg scale, nested and MINSERT fixture passed");
    }
}
