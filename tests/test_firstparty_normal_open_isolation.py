"""Regression: an incomplete first-party AC1032 scene must never be
admitted by normal Android DWG open after mainline branch integration.

This tests wiring and fail-closed boundaries, NOT authentic DWG geometry.
The separate opt-in diagnostic remains available for explicit experiments.
"""
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "app/src/main/java/com/musa/cad"
JNI = ROOT / "app/src/main/cpp"


class FirstPartyNormalOpenIsolation(unittest.TestCase):
    def test_normal_open_does_not_admit_partial_scene(self):
        source = (JAVA / "MainActivity.java").read_text(encoding="utf-8")
        method = source.split("private void startLoad(Uri uri){", 1)[1].split(
            "\n    private void ", 1
        )[0]
        self.assertIn("NativeCadEngine.open(loaded.file)", method)
        for forbidden in (
            "FirstPartyDwgPreview.read(",
            "FirstPartyDwgPreview.readAllInsert",
            "FirstPartyDwgPreview.readCombined",
            "FirstPartyDwgPreview.Result actual=",
            "GERÇEK AC1032 • SALT OKUNUR • EKSİK SAHNE",
            "project.nativeScene=realScene",
            "loaded.nativeScene=realScene",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, method)

    def test_normal_open_jni_remains_fail_closed(self):
        source = (JNI / "firstparty_dwg_unavailable_jni.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("Java_com_musa_cad_NativeCadEngine_openNative", source)
        self.assertIn("java/io/IOException", source)
        self.assertIn("unsupported(env);", source)
        self.assertNotIn("dwg_read_file(", source)
        self.assertNotIn("dwg_write_file(", source)

    def test_standalone_diagnostic_not_editor_wiring(self):
        main = (JAVA / "MainActivity.java").read_text(encoding="utf-8")
        dialog = (JAVA / "FirstPartyDwgPreviewDialog.java").read_text(
            encoding="utf-8"
        )
        self.assertIn("previewStandaloneOriginalDwg(Uri uri)", main)
        self.assertIn("FirstPartyDwgPreviewDialog.show(this,copied,true)", main)
        self.assertIn("FirstPartyDwgPreview.read(source,MAX_FLOAT_WORDS)", dialog)
        self.assertNotIn("NativeCadEngine.open(", dialog)
        self.assertNotIn("setNativeDrawing(", dialog)
        self.assertNotIn("saveDxf(", dialog)


if __name__ == "__main__":
    unittest.main()
