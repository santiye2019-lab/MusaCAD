"""Synthetic regression: host spline diagnostics must never become a false DWG PASS.

No real drawings are opened and no source DWGs are sent to CI.
"""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from private_dwg_read_audit import convert_one


DXF = """0
SECTION
2
ENTITIES
0
LINE
8
0
10
0
20
0
11
1
21
1
0
ENDSEC
0
EOF
"""


class NativeSplineDiagnosticAuditTest(unittest.TestCase):
    def inspect(self, stderr, exit_code=0):
        with tempfile.TemporaryDirectory() as dirname:
            temp = Path(dirname)
            source = temp / "synthetic.dwg"
            source.write_bytes(b"AC1024\x00synthetic")
            def synthetic_converter(command, *, stdout, stderr, **kwargs):
                Path(command[2]).write_text(DXF, encoding="utf-8")
                stderr.write(log.encode("utf-8"))
                return subprocess.CompletedProcess(command, exit_code)
            log = stderr
            with patch("private_dwg_read_audit.subprocess.run",
                       side_effect=synthetic_converter):
                return convert_one(source, Path("/not-executed/converter"),
                                   temp, timeout=2, limit_mb=64)

    def test_complete_clean_scene_is_only_accepted_case(self):
        result = self.inspect(
            "MusaCAD native scene: status=0 primitives=1 floats=12\n")
        self.assertEqual(result["status"], "decoded_dxf_inventory")
        self.assertEqual(result["native_spline_rejection_count"], 0)
        self.assertFalse(result["native_scene_truncated"])

    def test_native_spline_rejection_never_passes_on_zero_exit(self):
        result = self.inspect(
            "MusaCAD native scene: status=0 primitives=1 floats=12\n"
            "MusaCAD spline rejected: scenario=2 degree=3 control=0 "
            "knots=0 fit=6 weighted=0 rational=0 "
            "reason=missing_knots_or_fit_only ctrl_pointer=0 "
            "knots_pointer=0 closed=0 periodic=0 "
            "begin_tangent_zero=1 end_tangent_zero=1\n"
            "PRIVATE_COORDINATE_DATA_MUST_NOT_LEAK\n")
        self.assertEqual(result["status"],
                         "decoded_dxf_inventory_scene_incomplete")
        self.assertEqual(result["native_spline_rejection_count"], 1)
        detail = result["native_spline_rejection_details"][0]
        self.assertEqual((detail["scenario"], detail["degree"],
                          detail["fit"]), (2, 3, 6))
        self.assertEqual(detail["ctrl_pointer"], 0)
        self.assertNotIn("PRIVATE_COORDINATE_DATA_MUST_NOT_LEAK",
                         json.dumps(result))

    def test_fit_rejection_stages_are_typed_and_fail_closed(self):
        result = self.inspect(
            "MusaCAD native scene: status=0 primitives=1 floats=12\n"
            "MusaCAD fit spline rejected: stage=build fit=5 "
            "duplicate_adjacent=1 nonfinite_fit=0 "
            "begin_tangent_finite=1 end_tangent_finite=1\n"
            "MusaCAD fit spline rejected: stage=scene_budget fit=8\n")
        self.assertEqual(result["status"],
                         "decoded_dxf_inventory_scene_incomplete")
        self.assertEqual(result["native_spline_rejection_count"], 2)
        self.assertEqual(
            result["native_fit_spline_stage_diagnostics"],
            [{"stage": "build", "fit": 5, "duplicate_adjacent": 1,
              "nonfinite_fit": 0, "begin_tangent_finite": 1,
              "end_tangent_finite": 1},
             {"stage": "scene_budget", "fit": 8,
              "duplicate_adjacent": None, "nonfinite_fit": None,
              "begin_tangent_finite": None, "end_tangent_finite": None}])

    def test_nonzero_or_truncated_scene_status_cannot_pass(self):
        for status, expected in (
            (1, "decoded_dxf_inventory_scene_truncated"),
            (-2, "decoded_dxf_inventory_scene_incomplete"),
        ):
            with self.subTest(status=status):
                result = self.inspect(
                    f"MusaCAD native scene: status={status} "
                    "primitives=1 floats=12\n")
                self.assertEqual(result["status"], expected)

    def test_scene_status_missing_remains_unverified(self):
        result = self.inspect("No native scene record\n")
        self.assertEqual(result["status"],
                         "decoded_dxf_inventory_scene_unverified")


if __name__ == "__main__":
    unittest.main()
