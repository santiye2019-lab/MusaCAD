"""Source-path contract for the converted DWG 3D-only handoff.
Does not claim Autodesk 2026 or physical phone acceptance.
"""
from pathlib import Path
import unittest

MAIN = Path(__file__).resolve().parents[1] / "app/src/main/java/com/musa/cad/MainActivity.java"

class ThreeDOnlyDwgContract(unittest.TestCase):
    def test_converted_dwg_is_checked_after_export_and_before_2d_only_rejection(self):
        s = MAIN.read_text(encoding="utf-8")
        start = s.index("private void startLoad(Uri uri)")
        native = s.index("// Progressive DWG opening:", start)
        export = s.index("engine.exportDxf(converted)", native)
        handoff = s.index("if(loaded.threeDOnly){", export)
        block = s[export:handoff]
        self.assertIn("DxfParser.render(converted,false)", block)
        self.assertIn("if(parsed==null||parsed.entityCount<=0)", block)
        self.assertIn("Dxf3dMesh.read(converted)", block)
        self.assertIn("Cad3dOnlyOpenGuard.requireRealGeometry(threeD)", block)
        self.assertIn("loaded.threeDOnly=true", block)
        self.assertIn("loaded.workingDxf=converted", block)
        self.assertIn("keep=true", block)
        self.assertNotIn('throw new IOException("DWG içinde desteklenen 2B nesne bulunamadı")', block)

    def test_handoff_owns_staged_dxf_and_keeps_original_dwg_cache(self):
        s=MAIN.read_text(encoding="utf-8")
        start=s.index("if(loaded.threeDOnly){",s.index("engine.exportDxf(converted)"))
        end=s.index("if(loaded.handedOff&&loaded.project!=null)",start)
        section=s[start:end]
        for required in [
            "runOnUiThread", "projects.contains(project)", "if(activeLoad!=task)",
            "project.file=loaded.file", "project.workingDxf=loaded.workingDxf",
            "project.threeDOnly=true", "project.preparingEditor=false",
            "if(currentProject==project)", "open3d(Mesh3dActivity.MODE_ISO)",
        ]:
            self.assertIn(required,section)
        self.assertIn("if(!loaded.handedOff)loaded.dispose();",section)
        self.assertIn("loaded.workingDxf.delete();",section)
        self.assertNotIn("Mesh3dActivity.EXTRA_TRANSIENT_DXF,true",section)

    def test_original_2d_path_and_existing_dxf_guard_remain(self):
        s=MAIN.read_text(encoding="utf-8")
        start=s.index("if(loaded.dxf){",s.index("private void startLoad(Uri uri)"))
        end=s.index("// Progressive DWG opening:",start)
        direct=s[start:end]
        self.assertIn("Cad3dOnlyOpenGuard.requireRealGeometry(geometry)",direct)
        self.assertIn("Mesh3dActivity.EXTRA_TRANSIENT_DXF,true",direct)
        self.assertIn("Cad3dOnlyOpenGuard.requireRealGeometry(threeD)",s)
        self.assertIn("cad.upgradeNativeDrawing(parsed)",s)
        self.assertIn("currentProject.threeDOnly",s)

if __name__=="__main__":
    unittest.main()
