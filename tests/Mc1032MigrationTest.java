import com.musa.cad.Mc1032Native;
import com.musa.cad.Mc1032V2Native;
import com.musa.cad.Mc1032Migration;
import java.io.IOException;
import java.util.Arrays;

public final class Mc1032MigrationTest {
    private static long raw(double d){return Double.doubleToRawLongBits(d);}
    private static void check(boolean v,String message){if(!v)throw new AssertionError(message);}
    public static void main(String[]args)throws Exception{
        long[] old={
            30,1,raw(-10),raw(2.5),raw(3.25),raw(4),raw(0),raw(0),raw(0),
            20,4,raw(1.125),raw(2),raw(0),raw(0),raw(0),raw(0),raw(0)
        };
        byte[] source=Mc1032Native.encode(old);
        byte[] upgraded=Mc1032Migration.upgrade(source);
        Mc1032V2Native.Drawing scene=Mc1032V2Native.decode(upgraded);
        check(Arrays.equals(scene.names,new String[]{"0"}),"default layer required");
        check(scene.entities.length==18,"two records unchanged");
        check(scene.entities[0]==30&&scene.entities[1]==1&&scene.entities[2]==0,"line ID+layer");
        check(scene.entities[3]==raw(-10)&&scene.entities[4]==raw(2.5)&&
              scene.entities[5]==raw(0),"line WCS Z0");
        check(scene.entities[6]==raw(3.25)&&scene.entities[7]==raw(4)&&
              scene.entities[8]==raw(0),"line endpoint WCS Z0");
        check(scene.entities[9]==20&&scene.entities[10]==2&&
              scene.entities[11]==0,"point ID and type");
        check(scene.entities[12]==raw(1.125),"point X preserved");
        byte[] downgraded=Mc1032Migration.downgrade(upgraded);
        check(Arrays.equals(source,downgraded),"wire-identical v1 roundtrip");
        long[] circle={7,2,raw(0),raw(0),raw(0),raw(0),raw(5),raw(0),raw(0)};
        try{Mc1032Migration.upgrade(Mc1032Native.encode(circle));
            throw new AssertionError("circle silently lost");}
        catch(IOException expected){check(expected.getMessage().contains("kayipsiz"),"reject circle");}
        long[] geometry3d={33,1,0,raw(0),raw(0),raw(11),raw(5),raw(5),raw(12)};
        Mc1032V2Native.Drawing higher=new Mc1032V2Native.Drawing(
            new String[]{"0"},new int[]{-1},geometry3d);
        try{Mc1032Migration.downgrade(Mc1032V2Native.encode(higher));
            throw new AssertionError("Z silently lost");}
        catch(IOException expected){check(expected.getMessage().contains("kayipsiz"),"reject 3D");}
        higher=new Mc1032V2Native.Drawing(
            new String[]{"0","PIPES"},new int[]{-1,0x0088ffff},
            new long[]{44,2,1,raw(3),raw(2),raw(0),raw(0),raw(0),raw(0)});
        try{Mc1032Migration.downgrade(Mc1032V2Native.encode(higher));
            throw new AssertionError("named layers silently lost");}
        catch(IOException expected){check(expected.getMessage().contains("kayipsiz"),"reject layers");}
        check(Arrays.equals(source,Mc1032Migration.downgrade(upgraded)),
              "repeated conversion stable and deterministic");
        System.out.println("PASS: JNI native MC1032 lossless upgrade + refusal to drop layers or Z");
    }
}
