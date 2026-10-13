import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Protects 3D Android UCS selection and safe staged DXF export integration. */
public final class CadUcsUiContractTest {
    private static void verify(boolean yes,String text){if(!yes)throw new AssertionError(text);}
    public static void main(String[] args)throws Exception{
        String src=Files.readString(Path.of("app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        verify(src.contains("private CadUcsFrame editingUcs=CadUcsFrame.world()"),
            "WCS should be default editing coordinates");
        verify(src.contains("showUcsSelector(ucsButton)"),"3D editor must expose UCS selection");
        verify(src.contains("CadUcsFrame.front()")&&src.contains("CadUcsFrame.right()"),
            "preset front/right UCS must be selectable");
        verify(src.contains("CadUcsFrame.parse(input.getText().toString())"),
            "custom axes must be validated before applying");
        verify(src.contains("editingUcs.move(axis,sign*amount)"),
            "selected UCS must drive edit button movement");
        verify(src.contains("mesh.xyz[vertex*3+k]+delta[k]")&&
            src.contains("Math.abs(value)>1e9d"),
            "source XYZ must be bounds-checked and moved together in WCS");
        verify(src.contains("for(int k=0;k<3;k++)mesh.xyz[vertex*3+k]=next[k]"),
            "partial invalid axis delta must not corrupt geometry");
        verify(src.contains("Dxf3dSafeSave.stage(")&&src.contains("Dxf3dSafeSave.publish("),
            "UCS editor must preserve guarded staged 3D DXF saving");
        System.out.println("3D UCS controls and guarded DXF save UI integration contract passed");
    }
}
