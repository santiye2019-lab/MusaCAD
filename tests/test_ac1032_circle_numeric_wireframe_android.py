"""Positive genuine DWG CIRCLE numeric display is explicitly NOT source paint."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class CircleNumericSourceOnly(unittest.TestCase):
    def test_android_native_source_and_display_disclaimer(self):
        native=(ROOT/"app/src/main/cpp/ac1032_insert_circle_numeric_wireframe_jni.c").read_text()
        cmake=(ROOT/"app/src/main/cpp/CMakeLists.txt").read_text()
        java=(ROOT/"app/src/main/java/com/musa/cad/FirstPartyDwgPreview.java").read_text()
        dialog=(ROOT/"app/src/main/java/com/musa/cad/FirstPartyDwgPreviewDialog.java").read_text()
        self.assertIn("ac1032_insert_circle_numeric_wireframe_jni.c",cmake)
        self.assertIn("Java_com_musa_cad_FirstPartyDwgPreview_readCircleGeometryEvidenceNative",native)
        self.assertIn('fopen(path,"rb")',native)
        self.assertIn("musa_dwg_ac1032_circle_wireframe_v6_diagnostic(",native)
        self.assertIn("!first.final_color_visibility_and_ownership_accepted",native)
        self.assertIn("readCircleGeometryEvidenceNative(",java)
        self.assertIn("sourceColorKnown=false;complete=false;",java)
        self.assertIn("packet[i]!=518f||packet[i+1]!=65f",java)
        self.assertIn("readCircleGeometryEvidence(",dialog)
        self.assertIn("MOR ÇİZGİLER KAYNAK DWG RENGİ DEĞİLDİR",dialog)
        self.assertIn("H4/H5 sahiplik/katmanı belirsiz",dialog)
        self.assertIn("sceneCanvas.setScene(result.scene)",dialog)
        self.assertEqual(dialog.count("activeReaders.incrementAndGet()"),4)
        self.assertEqual(dialog.count("activeReaders.decrementAndGet()"),5)
        self.assertIn("if(temporaryCopy&&dismissed.get()&&activeReaders.get()==0)",dialog)
        for word in ('fopen(path,"wb")',"dwg_read_file(","dwg_write_file("):
            self.assertNotIn(word,native)
        for word in ("NativeCadEngine.open(","setNativeDrawing(","replaceVisibleDrawing("):
            self.assertNotIn(word,dialog)
    def test_three_source_scene_keeps_magenta_non_source(self):
        java=(ROOT/"app/src/main/java/com/musa/cad/FirstPartyDwgPreview.java").read_text()
        native_scene=(ROOT/"app/src/main/java/com/musa/cad/NativeScene.java").read_text()
        merger=(ROOT/"app/src/main/java/com/musa/cad/OriginalSourceV6Merge.java").read_text()
        dialog=(ROOT/"app/src/main/java/com/musa/cad/FirstPartyDwgPreviewDialog.java").read_text()
        host=(ROOT/"tests/OriginalSourceV6MergeTest.java").read_text()
        self.assertIn("readCombinedWithNumericCirclesDiagnostic(",java)
        self.assertIn("readCircleGeometryEvidence(",java)
        self.assertIn("sourceCircleColorAccepted=false",java)
        self.assertIn("joinPartialOriginalSourceScenesWithDiagnosticCircles(",native_scene)
        self.assertIn("joinWithDiagnosticCircles(",merger)
        self.assertIn("numericCircles[at]!=518f",merger)
        self.assertIn("numericCircles[at+1]!=65f",merger)
        self.assertIn("CIRCLE WCS noktasi kaynak sinirini asti",merger)
        self.assertIn("MOR CIRCLE ÇİZGİLERİ KAYNAK DWG RENGİ DEĞİLDİR",dialog)
        self.assertIn("readCombinedWithNumericCirclesDiagnostic(",dialog)
        self.assertIn("Fake original CIRCLE ACI accepted",host)
        self.assertIn("Open numeric CIRCLE accepted",host)
        self.assertIn("Over-budget 3-source packet accepted",host)
        self.assertNotIn("sourceCircleColorAccepted=true",java)

    def test_real_r2018_positive_numeric_not_full_display(self):
        header=(ROOT/"core/native/include/musacad_dwg_ac1032_insert_circle_numeric_wireframe_v6.h").read_text()
        self.assertIn("MUSACAD_AC1032_CIRCLE_DIAGNOSTIC_ACI 6u",header)
        self.assertIn("original_numeric_source_circle_axes_verified",header)
        self.assertIn("source_only_partial=1u",header)
        self.assertIn("paint_is_diagnostic_not_source=1u",header)
        self.assertIn("final_color_visibility_and_ownership_accepted",header)
        self.assertNotIn("source_aci=s",header)
if __name__=="__main__":unittest.main()
