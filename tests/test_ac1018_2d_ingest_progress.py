"""Evidence-backed scoped AC1018 2D ingest milestone coverage.

Do NOT conflate with the 25-gate whole-core index or AutoCAD parity.
A merged PR number alone is not evidence: local source and native test must
both remain present. Workflow checks give independent compile/CTest proof.
"""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROGRESS = ROOT / "docs" / "MUSACAD_AC1018_2D_INGEST_PROGRESS.json"
GATES = ROOT / "docs" / "MUSACAD_CORE_COMPLETION_GATES.json"


class Ac1018IngestProgressTest(unittest.TestCase):
    def test_fixed_denominator_unique_milestones_and_source_evidence(self):
        report = json.loads(PROGRESS.read_text(encoding="utf-8"))
        whole = json.loads(GATES.read_text(encoding="utf-8"))
        items = report["milestones"]
        self.assertEqual(1, report["schema_version"])
        self.assertEqual(16, len(items))
        self.assertEqual(16, report["total_milestones"])
        self.assertEqual(len(items), len({item["id"] for item in items}))
        self.assertTrue(all(item["status"] in ("verified", "pending") for item in items))
        accepted = [item for item in items if item["status"] == "verified"]
        self.assertEqual(len(accepted), report["verified_milestones"])
        self.assertEqual(len(accepted) * 100 / 16, report["coverage_percent"])
        self.assertIsNone(report["autoCAD_2026_parity_percent"])
        self.assertNotEqual(report["metric_kind"], "autoCAD_2026_parity_percent")
        self.assertEqual("docs/MUSACAD_CORE_COMPLETION_GATES.json",
                         report["full_core_gate_coverage_reference"]["path"])
        gate_ref = report["full_core_gate_coverage_reference"]
        self.assertEqual(whole["total_gates"], gate_ref["total"])
        self.assertEqual(whole["accepted_gates"], gate_ref["last_verified_accepted"])
        self.assertEqual(whole["completion_index_percent"], gate_ref["percent"])
        for item in accepted:
            self.assertIsInstance(item["merged_pr"], int)
            self.assertGreater(item["merged_pr"], 0)
            self.assertEqual(2, len(item["evidence"]))
            for rel in item["evidence"]:
                self.assertTrue((ROOT / rel).is_file(),
                                "Milestone %s missing source/test %s" % (item["id"], rel))
        for item in items:
            if item["status"] == "pending":
                self.assertNotIn("merged_pr", item)
                self.assertNotIn("evidence", item)

    def test_scope_is_not_advertised_as_whole_cad_completion(self):
        report = json.loads(PROGRESS.read_text(encoding="utf-8"))
        self.assertIn("workstream", report["metric_kind"])
        self.assertIn("not_core_completion", report["metric_kind"])
        self.assertIn("not_autocad_parity", report["metric_kind"])
        self.assertIn("NOT", report["method"])
        self.assertNotIn("full_core_completion_percent", report)

if __name__ == "__main__":
    unittest.main()
