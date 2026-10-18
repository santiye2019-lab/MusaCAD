"""Executable observable-reference contract. NO LibreDWG code, SDK or runtime use.

A reference format can be known to LibreDWG without being implemented in MusaCAD.
All modern DWG writer and three-platform certification claims must remain unearned.
"""
import json
import re
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
MATRIX=ROOT/"docs/DWG_LIBREDWG_REFERENCE_MATRIX.json"
SIGNATURE=ROOT/"core/native/src/musacad_dwg_signature.c"
SIG_TEST=ROOT/"core/native/tests/core_dwg_signature_test.c"
HEADER=ROOT/"core/native/include/musacad_dwg_signature.h"
ACCEPT=ROOT/"docs/AUTOCAD_2026_ACCEPTANCE_51.json"
POLICY=ROOT/"AGENTS.md"

class FunctionalReferenceMatrixTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m=json.loads(MATRIX.read_text(encoding="utf-8"))
        cls.accept=json.loads(ACCEPT.read_text(encoding="utf-8"))

    def test_each_native_signature_is_tracked_as_header_only(self):
        versions=self.m["version_matrix"]
        self.assertEqual(len(versions),9)
        sig=SIGNATURE.read_text(encoding="utf-8")
        header=HEADER.read_text(encoding="utf-8")
        checked=SIG_TEST.read_text(encoding="utf-8")
        self.assertEqual(len({v["magic"] for v in versions}),len(versions))
        for v in versions:
            magic=v["magic"]
            self.assertRegex(magic,r"^AC[0-9]{4}$")
            self.assertIn('memcmp(bytes,"'+magic+'",6)',sig)
            self.assertIn(v["native_signature_enum"],header)
            self.assertIn('recognized("'+magic+'",',checked)
            self.assertEqual(v["musacad_signature_identification"],"implemented_tested")
            self.assertEqual(v["musacad_real_binary_decode"],"not_implemented")
            self.assertEqual(v["musacad_real_binary_encode"],"not_implemented")
            self.assertFalse(v["independent_acceptance"])
            self.assertEqual(v["roundtrip_evidence"],"not_run")
            self.assertEqual(v["fixture_evidence"],"not_collected")

    def test_reference_writer_cannot_be_mislabelled_compatible(self):
        versions={v["magic"]:v for v in self.m["version_matrix"]}
        self.assertEqual(versions["AC1015"]["libredwg_write_reference"],"recommended")
        self.assertEqual(versions["AC1021"]["libredwg_write_reference"],"unsupported")
        for magic in ["AC1018","AC1024","AC1027","AC1032"]:
            self.assertEqual(versions[magic]["libredwg_write_reference"],"experimental")
        self.assertTrue(all(v["libredwg_read_reference"]=="documented_broad_support_with_known_gaps"
                            for v in versions.values()))

    def test_core_primitives_are_not_dwg_binary_decoder_evidence(self):
        stages={s["id"]:s for s in self.m["implementation_stages"]}
        self.assertEqual(stages["signature_identification"]["state"],"implemented_tested")
        self.assertEqual(stages["bounded_binary_bit_io"]["state"],"generic_infrastructure_tested")
        for key in ["dwg_section_page_map","dwg_section_checksums_and_compression",
                    "dwg_header_variables_and_classes","dwg_handles_and_object_database",
                    "dwg_entity_layer_layout_and_block_semantics",
                    "dwg_xref_proxy_dynamic_content","dwg_brep_acis_solid_semantics",
                    "dwg_native_binary_writer_and_rollback"]:
            self.assertEqual(stages[key]["state"],"not_implemented")
            self.assertIsNone(stages[key]["code"])
        self.assertEqual(stages["independent_dwg_reopen_and_full_loss_audit"]["state"],"not_run")

    def test_acceptance_51_gates_remain_unearned(self):
        self.assertEqual(self.m["autocad_2026_certified_families"],0)
        self.assertEqual(self.m["autocad_2026_family_denominator"],51)
        self.assertEqual(len(self.accept["cases"]),51)
        for case in self.accept["cases"]:
            if case["id"] in ("file_01","file_02"):
                self.assertEqual(case["review"]["decision"],"pending")
                self.assertTrue(all(v["status"]=="not_run" for v in case["gates"].values()))

    def test_libredwg_is_not_our_runtime(self):
        s=self.m["reference_policy"]
        for requirement in ("host-only","no LibreDWG code","first-party engine"):
            self.assertIn(requirement,s)
        self.assertIn("zero LibreDWG",POLICY.read_text(encoding="utf-8"))
        self.assertEqual(self.m["platform_core"],"core/native/")
        self.assertIn("iPhoneOS arm64",self.m["platform_targets"])
        self.assertIn("Windows",self.m["platform_targets"])
        self.assertIn("Android arm64-v8a",self.m["platform_targets"])

if __name__=="__main__":
    unittest.main()
