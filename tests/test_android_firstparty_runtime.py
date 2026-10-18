"""The Android app must not ship or call any external CAD SDK.

This is a source/link-graph acceptance guard. A full ELF dependency inspection
needs a signed-off Android native build later; it is not simulated here.
"""
import re
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
NATIVE=ROOT/"app/src/main/cpp"

class AndroidFirstPartyRuntime(unittest.TestCase):
    def test_android_library_has_no_external_cad_decoder(self):
        cmake=(NATIVE/"CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("if(ANDROID)",cmake)
        self.assertIn("return()\nendif()",cmake)
        block=cmake.split("if(ANDROID)",1)[1].split("return()",1)[0]
        executable="\n".join(line.split("#",1)[0] for line in block.splitlines())
        self.assertIn("add_library(musacad_dwg SHARED",executable)
        self.assertIn("firstparty_dwg_unavailable_jni.c",executable)
        self.assertIn("target_link_libraries(musacad_dwg PRIVATE m musacad_core)",executable)
        for forbidden in ("redwg","libredwg","dwg_bridge.c","native_scene.c",
                          "third_party","add_subdirectory(${LIBREDWG_SOURCE}"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden.lower(),executable.lower())

    def test_missing_own_dwg_codec_fails_closed_not_false_green(self):
        source=(NATIVE/"firstparty_dwg_unavailable_jni.c").read_text(encoding="utf-8")
        self.assertIn("Java_com_musa_cad_NativeCadEngine_openNative",source)
        self.assertIn("java/io/IOException",source)
        self.assertIn("unsupported(env);",source)
        self.assertIn("Java_com_musa_cad_NativeDwg_convertNative",source)
        self.assertNotIn("dwg_read_file(",source)
        self.assertNotIn("dwg_write_file(",source)
        self.assertNotRegex(source,r"#\s*include\s*[<\"](?:dwg|dwg_api|bits)\.h")

    def test_android_core_build_has_no_unproved_writers(self):
        cmake=(NATIVE/"CMakeLists.txt").read_text(encoding="utf-8")
        android=cmake.split("if(ANDROID)",1)[1].split("return()",1)[0]
        executable="\n".join(line.split("#",1)[0] for line in android.splitlines())
        self.assertNotIn("dwg_write_pilot.c",executable)
        self.assertIn("mesh_mass_jni.c",executable)
        self.assertIn("mesh_surface_jni.c",executable)
        self.assertIn("dxf_byte_lines_jni.c",executable)

if __name__=="__main__":
    unittest.main()
