"""Layout and paper-space viewport comparison; no project metadata leakage."""
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from dwg_layout_viewport_audit import DwgLayoutViewportAudit
from private_dwg_read_audit import convert_one, inventory_dxf, main


def rec(kind, *groups):
    return "0\n" + kind + "\n" + "".join(f"{k}\n{v}\n" for k, v in groups)


def layout_ref():
    return [{"handle": "A", "name": "PRIVATE_PAPER_LAYOUT",
             "groups": {"70": "1", "71": "2", "40": "297", "41": "210",
                        "340": "1b"}}]


def viewport_ref():
    return [{"handle": "B", "layout": "PRIVATE_PAPER_LAYOUT",
             "layer": "SECRET_LAYER", "space": "paperspace",
             "groups": {"10": "150", "20": "100", "40": "250", "41": "190",
                        "12": "0", "22": "0", "45": "100",
                        "51": "15", "69": "2", "331": ["F", "E"]}}]


def fixture():
    return (
        rec("SECTION", (2, "OBJECTS")) +
        rec("LAYOUT", (5, "0a"), (1, "PRIVATE_PAPER_LAYOUT"),
            (70, "1"), (71, "2"), (40, "297.0"), (41, "210"),
            (340, "1B")) +
        rec("ENDSEC") +
        rec("SECTION", (2, "ENTITIES")) +
        rec("VIEWPORT", (5, "B"), (8, "SECRET_LAYER"), (67, "1"),
            (410, "PRIVATE_PAPER_LAYOUT"),
            (10, "150"), (20, "100"), (40, "250"), (41, "190"),
            (12, "0"), (22, "0"), (45, "100"), (51, "15"),
            (69, "2"), (331, "E"), (331, "F")) +
        rec("LINE", (5, "C"), (8, "0"), (10, "0"), (20, "0"),
            (11, "2"), (21, "3")) +
        rec("ENDSEC") + rec("EOF")
    )


class LayoutViewportAuditTest(unittest.TestCase):
    def make_audit(self):
        audit = DwgLayoutViewportAudit()
        audit.observe("OBJECTS", "LAYOUT",
                      [(5, "0a"), (1, "PRIVATE_PAPER_LAYOUT"),
                       (70, "1"), (71, "2"), (40, "297"), (41, "210"),
                       (340, "1B")])
        audit.observe("ENTITIES", "VIEWPORT",
                      [(5, "B"), (8, "SECRET_LAYER"), (67, "1"),
                       (410, "PRIVATE_PAPER_LAYOUT"), (10, "150"),
                       (20, "100"), (40, "250"), (41, "190"),
                       (12, "0"), (22, "0"), (45, "100"), (51, "15"),
                       (69, "2"), (331, "E"), (331, "F")])
        return audit

    def test_valid_parity_and_private_metadata_not_in_report(self):
        audit = self.make_audit()
        report = audit.compare_independent(layout_ref(), viewport_ref())
        self.assertEqual(report["independent_layout_viewport_result"],
                         "layout_viewport_metadata_parity_only")
        self.assertEqual(report["independent_layouts_checked"], 1)
        self.assertEqual(report["independent_viewports_checked"], 1)
        for secret in ("PRIVATE_PAPER_LAYOUT", "SECRET_LAYER", "1B"):
            self.assertNotIn(secret, str(report))
        self.assertIn("not rendered paper-space", report["independent_layout_viewport_scope"])

    def test_changed_paper_size_view_and_layout_fail(self):
        audit = self.make_audit()
        for code, value in (("10", "151"), ("20", "99"),
                            ("40", "180"), ("41", "181"),
                            ("45", "101"), ("51", "16"),
                            ("69", "3"), ("12", "1")):
            changed = viewport_ref()
            changed[0]["groups"][code] = value
            self.assertEqual(audit.compare_independent(layout_ref(), changed)[
                "independent_viewport_property_mismatches"], 1)
        changed = viewport_ref()
        changed[0]["groups"]["331"] = ["D", "E"]
        self.assertEqual(audit.compare_independent(layout_ref(), changed)[
            "independent_viewport_property_mismatches"], 1)
        changed = layout_ref()
        changed[0]["groups"]["40"] = "594"
        self.assertEqual(audit.compare_independent(changed, viewport_ref())[
            "independent_layout_property_mismatches"], 1)
        changed = viewport_ref()
        changed[0]["layout"] = "NOT_PRESENT"
        report = audit.compare_independent(layout_ref(), changed)
        self.assertEqual(report["independent_layout_viewport_result"],
                         "layout_viewport_metadata_mismatch")

    def test_corruption_duplicate_or_missing_records_fail_closed(self):
        audit = self.make_audit()
        with self.assertRaises(ValueError):
            audit.compare_independent([], [])
        for bad in ({"40": "0"}, {"40": "nan"}, {"41": "-3"},
                    {"10": "Infinity"}, {"331": ["F", "F"]}):
            changed = viewport_ref()
            changed[0]["groups"].update(bad)
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                audit.compare_independent(layout_ref(), changed)
        changed = viewport_ref()
        changed[0]["groups"]["999"] = "unknown"
        with self.assertRaises(ValueError):
            audit.compare_independent(layout_ref(), changed)
        with self.assertRaises(ValueError):
            audit.compare_independent(layout_ref() * 2, viewport_ref())
        self.assertEqual(audit.compare_independent(layout_ref(), [])[
            "independent_layout_viewport_result"], "layout_viewport_metadata_mismatch")
        corrupt = self.make_audit()
        corrupt.observe("ENTITIES", "VIEWPORT", [
            (5, "C"), (8, "SECRET_LAYER"), (410, "PRIVATE_PAPER_LAYOUT"),
            (10, "1"), (20, "1"), (40, "0"), (41, "20")])
        self.assertEqual(corrupt.invalid_viewports, 1)
        corrupt.observe("ENTITIES", "VIEWPORT", [
            (5, "B"), (8, "SECRET_LAYER"), (410, "PRIVATE_PAPER_LAYOUT"),
            (10, "1"), (20, "1"), (40, "10"), (41, "20")])
        self.assertEqual(corrupt.duplicate_viewports, 1)

    def test_dxf_pipeline_and_status(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            dxf = root / "converted.dxf"
            dxf.write_text(fixture(), encoding="utf-8")
            audit = DwgLayoutViewportAudit()
            inventory_dxf(dxf, layout_collector=audit)
            self.assertEqual(audit.compare_independent(layout_ref(), viewport_ref())[
                "independent_layout_viewport_result"], "layout_viewport_metadata_parity_only")
            orig = root / "readonly.dwg"
            orig.write_bytes(b"AC1032 mocked private fixture")
            exe = root / "converter"
            exe.write_text("#!/bin/sh\nexit 0\n")
            exe.chmod(0o700)

            def conversion(args, **kwargs):
                Path(args[2]).write_text(fixture(), encoding="utf-8")
                kwargs["stderr"].write(
                    b"MusaCAD native scene: status=0 primitives=2 floats=10\n")
                return Mock(returncode=0)

            with patch("private_dwg_read_audit.subprocess.run", side_effect=conversion):
                with tempfile.TemporaryDirectory(dir=root) as tmp:
                    good = convert_one(orig, exe, Path(tmp), 5, 128,
                                       independent_layouts=layout_ref(),
                                       independent_viewports=viewport_ref())
                wrong = viewport_ref()
                wrong[0]["groups"]["40"] = "99"
                with tempfile.TemporaryDirectory(dir=root) as tmp:
                    bad = convert_one(orig, exe, Path(tmp), 5, 128,
                                      independent_layouts=layout_ref(),
                                      independent_viewports=wrong)
            self.assertEqual(good["status"], "decoded_dxf_inventory")
            self.assertEqual(bad["status"],
                             "decoded_dxf_inventory_layout_viewport_mismatch")
            self.assertEqual(orig.read_bytes(), b"AC1032 mocked private fixture")

    def test_manifest_requires_layout_and_viewport_arrays(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            original = root / "original.dwg"
            original.write_bytes(b"AC1032 layout manifest sample")
            digest = hashlib.sha256(original.read_bytes()).hexdigest()
            refs = root / "independent"
            refs.mkdir()
            reference_path = refs / (digest + ".json")
            exe = root / "converter"
            exe.write_text("#!/bin/sh\nexit 0\n")
            exe.chmod(0o700)
            manifest = {"source_sha256": digest,
                        "source_kind": "independent_dwg_reader",
                        "records": [], "layout_records": layout_ref(),
                        "viewport_records": viewport_ref()}
            reference_path.write_text(json.dumps(manifest), encoding="utf-8")
            argv = ["private_dwg_read_audit.py", "--converter", str(exe),
                    "--independent-manifest-dir", str(refs),
                    "--report", str(root / "report.json"), str(original)]
            with patch.object(sys, "argv", argv), patch(
                    "private_dwg_read_audit.convert_one",
                    return_value={"status": "decoded_dxf_inventory"}) as mocked:
                self.assertEqual(main(), 0)
                self.assertEqual(mocked.call_args.kwargs["independent_layouts"],
                                 layout_ref())
                self.assertEqual(mocked.call_args.kwargs["independent_viewports"],
                                 viewport_ref())
            manifest.pop("viewport_records")
            reference_path.write_text(json.dumps(manifest), encoding="utf-8")
            with patch.object(sys, "argv", argv), patch(
                    "private_dwg_read_audit.convert_one") as mocked:
                self.assertEqual(main(), 1)
                mocked.assert_not_called()


if __name__ == "__main__":
    unittest.main()
