"""The user-invoked original AC1032 preview cannot replace editable DWG state.

The diagnostic is an opt-in read-only Android UI adapter, NOT a way to
declare the first-party binary DWG document reader complete.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
APP=ROOT/"app/src/main/java/com/musa/cad"

class FirstPartyOriginalDwgDiagnosticIsolation(unittest.TestCase):
    def test_menu_has_independent_saf_picker_even_when_normal_open_is_unavailable(self):
        main=(APP/"MainActivity.java").read_text(encoding="utf-8")
        self.assertIn("MENU_ORIGINAL_DWG_2D_TEST=12",main)
        self.assertIn("case MENU_ORIGINAL_DWG_2D_TEST:showOriginalFirstPartyPreview()",main)
        self.assertIn("PICK_ORIGINAL_DWG_2D=40",main)
        self.assertIn("new Intent(Intent.ACTION_OPEN_DOCUMENT)",main)
        self.assertIn("Intent.CATEGORY_OPENABLE",main)
        self.assertIn("startActivityForResult(intent,PICK_ORIGINAL_DWG_2D)",main)
        self.assertIn("previewStandaloneOriginalDwg(data.getData())",main)
        self.assertIn('menu.add(0,MENU_ORIGINAL_DWG_2D_TEST,11,',main)
        self.assertIn('menu.add(0,MENU_MY_LICENSE,9,"Lisans Bilgilerim")',main)
        self.assertNotIn('"Bağımsız DWG 2B test görünümü (kısmi)").setEnabled(',main)
        method=main.split("private void showOriginalFirstPartyPreview(){",1)[1].split(
            "\n    private void ",1)[0]
        self.assertIn("Intent.ACTION_OPEN_DOCUMENT",method)
        copy=main.split("private void previewStandaloneOriginalDwg(Uri uri){",1)[1].split(
            "\n    private void ",1)[0]
        self.assertIn("getContentResolver().openInputStream(uri)",copy)
        self.assertIn("File.createTempFile(",copy)
        self.assertIn("FileTransfer.copy(in,out,64L*1024L*1024L,null)",copy)
        self.assertIn("FirstPartyDwgPreviewDialog.show(this,copied,true)",copy)
        self.assertIn("copied.delete()",copy)
        self.assertIn("temporary.delete()",copy)
        self.assertNotIn("cad.setNativeDrawing",method)
        self.assertNotIn("cad.replaceVisibleDrawing",method)
        self.assertNotIn("project.nativeScene=",method)
        self.assertNotIn("workingDxf=",method)

    def test_dialog_is_always_readonly_partial_and_off_ui_thread(self):
        source=(APP/"FirstPartyDwgPreviewDialog.java").read_text(encoding="utf-8")
        self.assertIn("FirstPartyDwgPreview.read(source,MAX_FLOAT_WORDS)",source)
        self.assertIn("new Thread(()->",source)
        self.assertIn("runOnUiThread",source)
        self.assertIn("preview.readiness",source)
        self.assertIn("KISMİ",source)
        self.assertIn("DENEYSEL",source)
        self.assertIn("sceneCanvas.setScene(preview.scene)",source)
        self.assertIn("dismissed.get()",source)
        self.assertIn("if(temporaryCopy)source.delete()",source)
        self.assertIn("show(owner,original,false)",source)
        self.assertIn("ScaleGestureDetector",source)
        self.assertIn("scene.draw(canvas,contentToScreen)",source)
        for disallowed in ("NativeCadEngine.open(","exportDxf(","setNativeDrawing(",
                           "replaceVisibleDrawing(","saveDxf(","FileOutputStream(",
                           "FileWriter(","FileChannel.open(","libredwg","redwg"):
            with self.subTest(disallowed=disallowed):
                self.assertNotIn(disallowed,source)

    def test_original_jni_is_firstparty_and_stays_optin(self):
        source=(APP/"FirstPartyDwgPreview.java").read_text(encoding="utf-8")
        self.assertIn("private static native float[] readOriginalNative",source)
        self.assertIn("NativeScene.fromRaw(raw)",source)
        self.assertIn("long[] report=new long[20]",source)
        self.assertIn("new Readiness(report)",source)
        self.assertIn("this.incomplete=true",source)
        self.assertIn("sourceStyleLinked+sourceStyleUnresolved!=planarRecognized",source)
        self.assertIn("previewAdmittedRecords+previewRejectedCandidates!=",source)
        self.assertIn("previewPrimitives!=parsed.primitiveCount",source)
        dialog=(APP/"FirstPartyDwgPreviewDialog.java").read_text(encoding="utf-8")
        self.assertIn("preview.readiness",dialog)
        self.assertIn("BYLAYER:",dialog)
        self.assertIn("BYBLOCK:",dialog)
        self.assertIn("KISMİ GÖRÜNÜM",dialog)
        self.assertIn("gerçekten önizlemeye alınan kaynak nesne",dialog)
        self.assertIn("başarı yüzdesi hesaplanamaz",dialog)
        self.assertIn("doğrulanmış doğrudan RGB24",dialog)
        self.assertIn("kaynakta gizli nesne",dialog)
        self.assertIn("diğer karmaşık stil",dialog)
        self.assertIn("katman açık/kapalı, donmuş",dialog)
        self.assertNotIn("önizleme tamamlandı (tüm DWG semantiği",dialog)
        self.assertIn("this.incomplete=true",source)
        native=(ROOT/"app/src/main/cpp/ac1032_firstparty_preview_jni.c").read_text(encoding="utf-8")
        self.assertIn("musa_dwg_ac1032_scene_index_open",native)
        self.assertIn("musa_dwg_ac1032_preview_v6_planar_source_aci",native)
        self.assertIn("musa_dwg_ac1032_preview_v6_source_census",native)
        self.assertIn("GetArrayLength(env,readinessOut)!=20",native)
        self.assertIn("const jlong values[20]",native)
        self.assertIn("fopen(path,\"rb\")",native)
        self.assertNotIn("fopen(path,\"wb\")",native)
        self.assertNotIn("dwg_read_file(",native)
        self.assertNotIn("dwg_write_file(",native)

if __name__=="__main__":
    unittest.main()
