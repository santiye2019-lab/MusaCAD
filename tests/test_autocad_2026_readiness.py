"""The readiness audit must never turn a green build into false parity."""
import copy
import json
import subprocess
import sys
import unittest
from pathlib import Path

from autocad_2026_51_acceptance import MATRIX, LEDGER, GATES
from autocad_2026_readiness import summarize


class AutoCAD2026ReadinessTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def test_every_family_has_an_explainable_readiness_record(self):
        result = summarize(self.matrix, self.ledger)
        self.assertEqual(result["family_count"], 51)
        self.assertEqual(len(result["families"]), 51)
        self.assertEqual(set(result["gate_pass_counts"]), set(GATES))
        self.assertEqual(result["scenarios_defined"],
                         sum(len(c["scenarios"]) for c in self.ledger["cases"]))
        self.assertEqual(result["scenario_inventories_incomplete"],
                         sum(not c["scenario_inventory_complete"]
                             for c in self.ledger["cases"]))
        self.assertEqual(result["certified_families"],
                         sum(c["review"]["decision"] == "approved"
                             for c in self.ledger["cases"]))
        self.assertEqual(result["reference_command_inventory_complete"],
                         self.matrix["scoring_policy"]["complete_catalog"])
        for case, output in zip(self.ledger["cases"], result["families"]):
            self.assertEqual(output["id"], case["id"])
            self.assertEqual(output["missing_gates"],
                             [g for g in GATES
                              if case["gates"][g]["status"] != "passed"])

    def test_unfinished_baseline_rejects_final_parity(self):
        result = summarize(self.matrix, self.ledger)
        self.assertEqual(result["readiness"], "blocked")
        self.assertGreater(result["scenario_inventories_incomplete"], 0)
        self.assertFalse(result["reference_command_inventory_complete"])
        self.assertNotEqual(result["certified_families"], result["family_count"])

    def test_invalid_report_cannot_be_counted_as_ready(self):
        for change in ("missing_family", "unverified_approval", "fake_green_ci"):
            ledger = copy.deepcopy(self.ledger)
            matrix = copy.deepcopy(self.matrix)
            if change == "missing_family":
                ledger["cases"].pop()
            elif change == "unverified_approval":
                ledger["cases"][0]["review"]["decision"] = "approved"
                matrix["capabilities"][0]["verified_reference_parity"] = True
                matrix["capabilities"][0]["implementation_status"] = "verified"
            else:
                ledger["cases"][0]["gates"]["continuous_integration"]["status"] = "passed"
            with self.subTest(change=change), self.assertRaises(ValueError):
                summarize(matrix, ledger)

    def test_final_ready_mode_fails_but_audit_mode_reports(self):
        script = Path(__file__).with_name("autocad_2026_readiness.py")
        cmd = [sys.executable, str(script)]
        status = subprocess.run(cmd, capture_output=True, text=True, check=False)
        self.assertEqual(status.returncode, 0, status.stderr)
        self.assertIn("Final parity readiness: BLOCKED", status.stdout)
        failed = subprocess.run(cmd + ["--require-ready"], capture_output=True,
                                text=True, check=False)
        self.assertEqual(failed.returncode, 1, failed.stderr)
        output = subprocess.run(cmd + ["--json"], capture_output=True,
                                text=True, check=False)
        self.assertEqual(output.returncode, 0, output.stderr)
        self.assertEqual(json.loads(output.stdout)["readiness"], "blocked")


if __name__ == "__main__":
    unittest.main()
