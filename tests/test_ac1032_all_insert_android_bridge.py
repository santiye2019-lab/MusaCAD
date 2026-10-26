"""Android multi-INSERT JNI renders only genuine, explicitly partial source evidence.

These tests assert the Android entry is linked, user-visible and uses the same
independent C11 packet proven by the actual R2018 DWG fixture test.
A passing test is NOT proof that all layers/colors/texts are correct.
"""
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CPP=ROOT/"app/src/main/cpp"
JAVA=ROOT/"app/src/main/java/com/musa/cad"

class MultiInsertVisibleDiagnostic(unittest.TestCase):
    def test_android_native_is_first_party_source_only(self):
        cmake=(CPP/"CMakeLists.txt").read_text(encoding="utf-8")
        android=cmake.split("if(ANDROID)",1)[1].split("return()",1)[0]
        self.assertIn("ac1032_all_insert_line_diagnostic_jni.c",android)
        self.assertIn("target_link_libraries(musacad_dwg PRIVATE m musacad_core)",android)
        source=(CPP/"ac1032_all_insert_line_diagnostic_jni.c").read_text(encoding="utf-8")
        self.assertIn("Java_com_musa_cad_FirstPartyDwgPreview_readAllInsertLinesNative",source)
        self.assertIn("musa_dwg_ac1032_all_insert_line_v6_diagnostic(",source)
        self.assertIn("musa_dwg_ac1032_scene_index_open(",source)
        self.assertIn("pre.source_packet_is_partial",source)
        self.assertIn("!pre.final_visibility_and_paint_accepted",source)
        self.assertIn("pre.verified_source_line_count==",source)
        self.assertIn("pre.admitted_line_count+pre.omitted_verified_line_count",source)
        self.assertIn('fopen(path,"rb")',source)
        for forbidden in ('fopen(path,"wb")',"dwg_read_file(","dwg_write_file(","libredwg","redwg","JNIEXPORT void JNICALL"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden,source)

    def test_java_never_marks_multi_insert_as_complete(self):
        source=(JAVA/"FirstPartyDwgPreview.java").read_text(encoding="utf-8")
        self.assertIn("private static native float[] readAllInsertLinesNative(",source)
        self.assertIn("AllInsertLinesDiagnostic readAllInsertLinesDiagnostic(",source)
        self.assertIn("audit[2]+audit[3]!=audit[1]",source)
        self.assertIn("audit[5]+audit[6]!=audit[4]",source)
        self.assertIn("audit[9]!=1||audit[10]!=0",source)
        self.assertIn("parsed=NativeScene.fromRaw(v6)",source)
        self.assertIn("parsed.primitiveCount!=audit[5]||!parsed.truncated",source)
        self.assertIn("incomplete=true;",source)
        self.assertIn("return new AllInsertLinesDiagnostic(parsed,audit)",source)

    def test_one_tap_scene_is_visible_but_explicitly_incomplete(self):
        source=(JAVA/"FirstPartyDwgPreviewDialog.java").read_text(encoding="utf-8")
        self.assertIn("Tüm doğrulanmış blok çizgilerini göster",source)
        self.assertIn("inspectAllBlocks.setOnClickListener(view->{",source)
        self.assertIn("FirstPartyDwgPreview.readAllInsertLinesDiagnostic(",source)
        self.assertIn("sceneCanvas.setScene(result.scene)",source)
        self.assertIn("result.admittedLines",source)
        self.assertIn("result.unsupportedInserts",source)
        self.assertIn("result.omittedLines",source)
        self.assertIn("BYLAYER/BYBLOCK",source)
        self.assertIn("YALNIZCA BLOK LINE KAYNAK TESTİ",source)
        self.assertIn("AtomicInteger activeReaders=new AtomicInteger(1)",source)
        self.assertEqual(source.count("activeReaders.incrementAndGet()"),2)
        self.assertEqual(source.count("activeReaders.decrementAndGet()"),3)
        self.assertIn("if(temporaryCopy&&dismissed.get()&&activeReaders.get()==0)",source)
        for forbidden in ("NativeCadEngine.open(","exportDxf(","setNativeDrawing(",
                          "replaceVisibleDrawing(","FileOutputStream(","libredwg","redwg"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden,source)

if __name__=="__main__":
    unittest.main()
