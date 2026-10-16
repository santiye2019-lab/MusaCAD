"""Source-contract regression for the DXF 3D-only Android handoff.
This does not replace physical-device acceptance or actual SAT/SAB tests.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "app/src/main/java/com/musa/cad/MainActivity.java"
MESH = ROOT / "app/src/main/java/com/musa/cad/Mesh3dActivity.java"

class ThreeDOnlyEntryTest(unittest.TestCase):
    def test_2d_empty_dxf_falls_back_to_real_mesh_preflight(self):
        s = MAIN.read_text(encoding="utf-8")
        start = s.index("if(loaded.dxf){", s.index("private void startLoad(Uri uri)"))
        end = s.index("// Progressive DWG opening:", start)
        section = s[start:end]
        self.assertIn("if(loaded.parsed==null||loaded.parsed.entityCount<=0)", section)
        self.assertIn("Dxf3dMesh.read(loaded.file)", section)
        self.assertIn("geometry.triangles.length==0&&geometry.edges.length==0", section)
        self.assertIn("Mesh3dActivity.EXTRA_TRANSIENT_DXF,true", section)
        self.assertIn("threeD.putExtra(Mesh3dActivity.EXTRA_DXF,loaded.file.getAbsolutePath())", section)
        self.assertIn("startActivity(threeD)", section)
        self.assertIn("loaded.dispose();", section)  # cancellation branch
        self.assertNotIn("Dxf3dMesh.read(uri)", section)

    def test_cleanup_is_scoped_to_explicit_private_cache_file_and_finish(self):
        s = MESH.read_text(encoding="utf-8")
        section = s[s.index("@Override protected void onDestroy()"):]
        for guard in (
            "isFinishing()",
            "getBooleanExtra(EXTRA_TRANSIENT_DXF,false)",
            "getCacheDir().getCanonicalFile()",
            'file.getName().startsWith("MusaCAD_acilan_")',
            "RECOVERY_IO.execute",
        ):
            self.assertIn(guard, section)
        self.assertNotIn("file.deleteOnExit()", section)

if __name__ == "__main__":
    unittest.main()
