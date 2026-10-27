"""Android CIRCLE source ACI audit must never become final visible paint.
The C11 inventory's authentic R2018 example_2018.dwg vs independently
parsed example_2018.dxf gold test is executed by the existing native
CMake and original DWG CI gates; this test guards its JNI/UI transport.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
class SourceAciAndroidEvidence(unittest.TestCase):
    def test_first_party_android_jni_and_no_source_paint(self):
        native=(ROOT/"app/src/main/cpp/ac1032_circle_layer_aci_evidence_jni.c").read_text()
        cmake=(ROOT/"app/src/main/cpp/CMakeLists.txt").read_text()
        java=(ROOT/"app/src/main/java/com/musa/cad/FirstPartyDwgPreview.java").read_text()
        dialog=(ROOT/"app/src/main/java/com/musa/cad/FirstPartyDwgPreviewDialog.java").read_text()
        header=(ROOT/"core/native/include/musacad_dwg_ac1032_circle_bylayer_inventory.h").read_text()
        gold=(ROOT/"core/native/tests/core_dwg_ac1032_circle_layer_dxf_status_oracle_test.c").read_text()
        self.assertIn("ac1032_circle_layer_aci_evidence_jni.c",cmake)
        self.assertIn("Java_com_musa_cad_FirstPartyDwgPreview_readCircleLayerAciEvidenceNative",native)
        self.assertIn('fopen(path,"rb")',native)
        self.assertIn('!memcmp(bytes,"AC1032",6u)',native)
        self.assertIn("musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory)",native)
        self.assertIn("!inventory.final_visible_paint_accepted",native)
        self.assertIn("original_layer_aci",native)
        self.assertIn("original_layer_packed_values",native)
        self.assertIn("readCircleLayerAciEvidenceNative(",java)
        self.assertIn("rows.length!=audit[7]*6L",java)
        self.assertIn("audit[8]!=1L||audit[9]!=0L",java)
        self.assertIn("finalVisiblePaintAccepted=false",java)
        self.assertIn("readCircleSourceAciEvidence(source)",dialog)
        self.assertIn("NİHAİ GÖRÜNÜRLÜK/RENK ONAYI YOK",dialog)
        self.assertIn("MOR ÇİZGİLER KAYNAK DWG RENGİ DEĞİLDİR",dialog)
        self.assertIn("MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX 256u",header)
        self.assertIn("inventory.verified_source_color_count>=1u",gold)
        self.assertIn("row->original_layer_aci==(uint8_t)oracle.aci",gold)
        self.assertIn("row->circle_handle==UINT64_C(0x1bb)",gold)
        for bad in ('fopen(path,"wb")',"dwg_read_file(","dwg_write_file(","redwg"):
            self.assertNotIn(bad,native)
        self.assertNotIn("sceneCanvas.setScene(colors",dialog)
        self.assertNotIn("sourceCircleColorAccepted=true",java)
if __name__=="__main__":
    unittest.main()
