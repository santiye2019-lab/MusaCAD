"""Synthetic tests for local-only original-DWG handle/layer parity audit."""
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from dwg_entity_identity_audit import DwgEntityIdentityAudit
from private_dwg_read_audit import inventory_dxf, main


def record(kind, *pairs):
    return "0\n" + kind + "\n" + "".join(
        f"{key}\n{value}\n" for key,value in pairs)


class DwgEntityIdentityAuditTest(unittest.TestCase):
    def test_duplicate_and_invalid_handles_are_counted_not_published(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE", [(5, "0a"), (8, "EQUIP SECRET")])
        audit.observe("ENTITIES", "CIRCLE", [(5, "A"), (8, "OTHER SECRET")])
        audit.observe("BLOCKS", "3DSOLID", [(5, "ZZ"), (8, "SENSITIVE")])
        audit.observe("BLOCKS", "LINE", [(8, "SENSITIVE")])
        audit.observe("ENTITIES", "ARC", [(5, "11"), (8, "")])
        audit.observe("ENTITIES", "POINT",
                      [(5, "12"), (5, "13"), (8, "0"), (8, "CUSTOM")])
        audit.observe("BLOCKS", "BLOCK", [(2, "PRIVATE")])
        s = audit.summary()
        self.assertEqual(s["decoded_identity_entity_records"], 6)
        self.assertEqual(s["decoded_identity_unique_handles"], 3)
        self.assertEqual(s["decoded_identity_duplicate_handles"], 1)
        self.assertEqual(s["decoded_identity_invalid_handles"], 1)
        self.assertEqual(s["decoded_identity_missing_handles"], 1)
        self.assertEqual(s["decoded_identity_missing_layers"], 1)
        self.assertEqual(s["decoded_identity_duplicate_handle_fields"], 1)
        self.assertEqual(s["decoded_identity_duplicate_layer_fields"], 1)
        self.assertEqual(s["decoded_identity_status"],
                         "decoded_records_have_identity_issues")
        self.assertNotIn("SECRET", str(s))
        self.assertNotIn("SENSITIVE", str(s))
        self.assertNotIn("PRIVATE", str(s))

    def test_independent_reference_exact_metadata_parity(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE", [(5, "A"), (8, "HVAC")])
        audit.observe("BLOCKS", "REGION", [(5, "00BF"), (8, "FIRE")])
        source = [
            {"handle":"0a", "space":"model", "type":"LINE", "layer":"HVAC"},
            {"handle":"bf", "space":"block", "type":"REGION", "layer":"FIRE"},
        ]
        comparison = audit.compare_independent(source)
        self.assertEqual(comparison["independent_identity_result"],
                         "identity_metadata_parity_only")
        self.assertEqual(comparison["independent_missing_in_converted"], 0)
        self.assertEqual(comparison["independent_unexpected_in_converted"], 0)
        self.assertNotIn("HVAC", str(comparison))
        self.assertNotIn("FIRE", str(comparison))
        self.assertNotIn("BF", str(comparison))

    def test_source_object_loss_and_semantic_drift(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "CIRCLE", [(5, "A"), (8, "CHANGED")])
        audit.observe("BLOCKS", "LINE", [(5, "B"), (8, "0")])
        source = [
            {"handle":"A", "space":"block", "type":"LINE", "layer":"ORIGINAL"},
            {"handle":"C", "space":"model", "type":"SPLINE", "layer":"0"},
        ]
        comparison = audit.compare_independent(source)
        self.assertEqual(comparison["independent_identity_result"],
                         "identity_metadata_mismatch")
        self.assertEqual(comparison["independent_missing_in_converted"], 1)
        self.assertEqual(comparison["independent_unexpected_in_converted"], 1)
        self.assertEqual(comparison["independent_space_mismatches"], 1)
        self.assertEqual(comparison["independent_type_mismatches"], 1)
        self.assertEqual(comparison["independent_layer_mismatches"], 1)

    def test_invalid_or_spoofed_reference_rejected(self):
        audit = DwgEntityIdentityAudit()
        for invalid in (
            None, {}, "FILE.dwg", [{"handle":"XYZ","space":"model","type":"LINE","layer":"0"}],
            [{"handle":"A","space":"model","type":"LINE","layer":"0"},
             {"handle":"000A","space":"model","type":"LINE","layer":"0"}],
            [{"handle":"A","space":"paper","type":"LINE","layer":"0"}],
            [{"handle":"A","space":"model","type":"LINE","layer":""}],
            [{"handle":"A","space":"model","type":"LINE","layer":"0","x":123}],
        ):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    audit.compare_independent(invalid)

    def test_model_paper_space_parity_and_no_sheet_name_leaks(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE",
                      [(5, "10"), (8, "SECRET MODEL"), (67, "0"), (410, "Model")])
        audit.observe("ENTITIES", "TEXT",
                      [(5, "11"), (8, "SECRET PAPER"), (67, "1"),
                       (410, "PRIVATE PAPER SHEET")])
        audit.observe("ENTITIES", "CIRCLE",
                      [(5, "12"), (8, "0"), (410, "CONFIDENTIAL LAYOUT")])
        audit.observe("BLOCKS", "LINE",
                      [(5, "13"), (8, "0"), (67, "1"), (410, "PRIVATE PAPER SHEET")])
        reference = [
            {"handle":"10","space":"model","type":"LINE","layer":"SECRET MODEL"},
            {"handle":"11","space":"paperspace","type":"TEXT","layer":"SECRET PAPER"},
            {"handle":"12","space":"paperspace","type":"CIRCLE","layer":"0"},
            {"handle":"13","space":"block","type":"LINE","layer":"0"},
        ]
        self.assertEqual(audit.summary()["decoded_identity_invalid_space_fields"], 0)
        result = audit.compare_independent(reference)
        self.assertEqual(result["independent_identity_result"],
                         "identity_metadata_parity_only")
        self.assertNotIn("PRIVATE", str(result))
        self.assertNotIn("SECRET", str(audit.summary()))
        modified = [dict(entry) for entry in reference]
        modified[1]["space"] = "model"
        mismatch = audit.compare_independent(modified)
        self.assertEqual(mismatch["independent_space_mismatches"], 1)
        self.assertEqual(mismatch["independent_identity_result"],
                         "identity_metadata_mismatch")

    def test_conflicting_or_malformed_paperspace_flags_fail_closed(self):
        invalid = (
            [(67, "2")], [(67, "X")], [(67, "1"), (67, "1")],
            [(67, "1"), (410, "Model")],
            [(67, "0"), (410, "PRIVATE LAYOUT")],
            [(410, "")], [(410, "A"), (410, "B")],
        )
        for extra in invalid:
            with self.subTest(extra=extra):
                audit = DwgEntityIdentityAudit()
                audit.observe("ENTITIES", "LINE", [(5, "A"), (8, "0")] + extra)
                status = audit.summary()
                self.assertEqual(status["decoded_identity_invalid_space_fields"], 1)
                self.assertEqual(status["decoded_identity_status"],
                                 "decoded_records_have_identity_issues")
                result = audit.compare_independent(
                    [{"handle":"A","space":"model","type":"LINE","layer":"0"}])
                self.assertEqual(result["independent_identity_result"],
                                 "identity_metadata_mismatch")

    def test_dxf_paperspace_inventory_hides_layout_name(self):
        source = (record("SECTION", (2, "ENTITIES")) +
                  record("LINE", (5, "A"), (8, "SECRET LAYER"),
                         (67, "1"), (410, "CONFIDENTIAL SHEET")) +
                  record("ENDSEC") + record("EOF"))
        with tempfile.TemporaryDirectory(prefix="musacad-dxf-layout-") as directory:
            path = Path(directory) / "fixture.dxf"
            path.write_text(source, encoding="ascii")
            inventory = inventory_dxf(path)
        self.assertEqual(inventory["decoded_identity_invalid_space_fields"], 0)
        self.assertEqual(inventory["decoded_identity_status"],
                         "decoded_identity_well_formed_source_unverified")
        identity_fields = {key: value for key, value in inventory.items()
                           if key.startswith("decoded_identity_")}
        self.assertNotIn("SECRET", str(identity_fields))
        self.assertNotIn("CONFIDENTIAL", str(identity_fields))

    def test_missing_decoded_layer_cannot_pass_even_if_reference_matches(self):
        audit = DwgEntityIdentityAudit()
        audit.observe("ENTITIES", "LINE", [(5, "1")])
        result = audit.compare_independent(
            [{"handle":"1","space":"model","type":"LINE","layer":"0"}])
        self.assertEqual(result["independent_identity_result"],
                         "identity_metadata_mismatch")
        self.assertEqual(result["independent_layer_mismatches"], 1)

    def test_manifest_sha_gate_refuses_unrelated_dwg_reference(self):
        with tempfile.TemporaryDirectory(prefix="musa-source-manifest-") as root:
            root = Path(root)
            source = root / "fixture.dwg"
            source.write_bytes(b"AC1032 synthetic readonly DWG signature")
            sha = hashlib.sha256(source.read_bytes()).hexdigest()
            manifest_dir = root / "refs"
            manifest_dir.mkdir()
            converter = root / "mock-converter"
            converter.write_text("#!/bin/sh\nexit 0\n", encoding="ascii")
            converter.chmod(0o700)
            report = root / "report.json"
            path = manifest_dir / (sha + ".json")
            manifest = {
                "source_sha256": "0" * 64,
                "source_kind": "independent_dwg_reader",
                "records": [
                    {"handle": "A", "space": "model",
                     "type": "LINE", "layer": "SECRET LAYER"}]}
            path.write_text(json.dumps(manifest), encoding="utf-8")
            args = ["private_dwg_read_audit.py", "--converter", str(converter),
                    "--independent-manifest-dir", str(manifest_dir),
                    "--report", str(report), str(source)]
            with patch.object(sys, "argv", args), patch(
                    "private_dwg_read_audit.convert_one") as convert:
                self.assertEqual(main(), 1)
                convert.assert_not_called()
            result = json.loads(report.read_text(encoding="utf-8"))["files"][0]
            self.assertEqual(result["status"], "invalid_independent_manifest")
            self.assertTrue(result["source_unchanged"])
            self.assertNotIn("SECRET LAYER", report.read_text(encoding="utf-8"))

            manifest["source_sha256"] = sha
            path.write_text(json.dumps(manifest), encoding="utf-8")
            with patch.object(sys, "argv", args), patch(
                    "private_dwg_read_audit.convert_one",
                    return_value={"status":"decoded_dxf_inventory"}) as convert:
                self.assertEqual(main(), 0)
                self.assertEqual(convert.call_args.args[-1], manifest["records"])

    def test_report_output_cannot_replace_original_dwg_or_symlink_alias(self):
        with tempfile.TemporaryDirectory(prefix="musa-report-safety-") as directory:
            folder = Path(directory)
            source = folder / "source.dwg"
            original = b"AC1015 synthetic drawing that must remain byte-identical"
            source.write_bytes(original)
            alias = folder / "report-alias.json"
            alias.symlink_to(source)
            for report_path in (source, alias):
                args = ["private_dwg_read_audit.py", "--manifest-only",
                        "--report", str(report_path), str(source)]
                with patch.object(sys, "argv", args), patch("sys.stderr"):
                    with self.assertRaises(SystemExit) as caught:
                        main()
                self.assertEqual(caught.exception.code, 2)
                self.assertEqual(source.read_bytes(), original)
            self.assertTrue(alias.is_symlink())

    def test_report_output_cannot_replace_external_source_manifest(self):
        with tempfile.TemporaryDirectory(prefix="musa-reference-safety-") as directory:
            folder = Path(directory)
            source = folder / "fixture.dwg"
            original = b"AC1015 synthetic readonly fixture"
            source.write_bytes(original)
            manifest_dir = folder / "refs"
            manifest_dir.mkdir()
            digest = hashlib.sha256(original).hexdigest()
            reference_path = manifest_dir / (digest + ".json")
            manifest = {"source_sha256": digest,
                        "source_kind": "independent_dwg_reader", "records": []}
            original_reference = json.dumps(manifest)
            reference_path.write_text(original_reference, encoding="utf-8")
            converter = folder / "mock-converter"
            converter.write_text("#!/bin/sh\\nexit 0\\n", encoding="ascii")
            converter.chmod(0o700)
            args = ["private_dwg_read_audit.py", "--converter", str(converter),
                    "--independent-manifest-dir", str(manifest_dir),
                    "--report", str(reference_path), str(source)]
            with patch.object(sys, "argv", args), patch("sys.stderr"), patch(
                    "private_dwg_read_audit.convert_one") as convert:
                with self.assertRaises(SystemExit) as caught:
                    main()
                convert.assert_not_called()
            self.assertEqual(caught.exception.code, 2)
            self.assertEqual(reference_path.read_text(encoding="utf-8"),
                             original_reference)
            self.assertEqual(source.read_bytes(), original)

    def test_synthetic_dxf_groups_5_8_audited_without_details(self):
        source = (
            record("SECTION",(2,"BLOCKS")) +
            record("BLOCK",(2,"PRIVATE-ROOM")) +
            record("3DSOLID",(5,"000A"),(8,"CONFIDENTIAL ROOM")) +
            record("ENDBLK") + record("ENDSEC") +
            record("SECTION",(2,"ENTITIES")) +
            record("LINE",(5,"B"),(8,"PROJECT PIPE")) +
            record("CIRCLE",(5,"B"),(8,"DIFFERENT LAYER")) +
            record("ARC",(5,"BAD_HANDLE!"),(8,"SECRET")) +
            record("ENDSEC") + record("EOF")
        )
        with tempfile.TemporaryDirectory(prefix="musacad-identity-") as tmp:
            path = Path(tmp) / "generated.dxf"
            path.write_text(source,encoding="ascii")
            result = inventory_dxf(path)
        self.assertEqual(result["decoded_identity_entity_records"], 4)
        self.assertEqual(result["decoded_identity_unique_handles"], 2)
        self.assertEqual(result["decoded_identity_duplicate_handles"], 1)
        self.assertEqual(result["decoded_identity_invalid_handles"], 1)
        self.assertEqual(result["native_preview_unsupported_entity_types"],
                         {"3DSOLID": 1})
        exposed = {k:v for k,v in result.items()
                   if k.startswith("decoded_identity_")}
        self.assertNotIn("CONFIDENTIAL", str(exposed))
        self.assertNotIn("PIPE", str(exposed))
        self.assertNotIn("000A", str(exposed))


if __name__ == "__main__":
    unittest.main()
