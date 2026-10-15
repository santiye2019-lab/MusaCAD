import copy
import json
import unittest
from pathlib import Path

from autocad_2026_parity_gate import MATRIX, ROOT, validate_matrix


class AutoCAD2026ParityGateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MATRIX.read_text(encoding="utf-8"))

    def test_real_matrix_is_unscored_pending_full_reference_catalogue(self):
        states = validate_matrix(self.data, ROOT)
        self.assertEqual(sum(states.values()), len(self.data["capabilities"]))
        self.assertEqual(states["verified"], 0)
        self.assertIsNone(self.data["scoring_policy"]["parity_percentage"])
        self.assertFalse(self.data["scoring_policy"]["complete_catalog"])
        self.assertGreaterEqual(len(self.data["capabilities"]), 45)

    def test_historic_planning_percent_cannot_become_parity(self):
        changed = copy.deepcopy(self.data)
        changed["scoring_policy"]["parity_percentage"] = 61
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)
        changed["scoring_policy"]["parity_percentage"] = None
        changed["scoring_policy"]["complete_catalog"] = True
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)

    def test_fake_verified_feature_without_real_reference_fails(self):
        changed = copy.deepcopy(self.data)
        item = changed["capabilities"][0]
        item["implementation_status"] = "verified"
        item["verified_reference_parity"] = True
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)
        item["parity_test_evidence"] = {
            "real_autocad_2026_result": "reference",
            "independent_real_dwg_cases": "DWG",
            "device_acceptance_cases": "",
            "regression_run": "build",
        }
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)

    def test_command_alias_alone_does_not_count_as_parity(self):
        changed = copy.deepcopy(self.data)
        item = changed["capabilities"][1]
        item["implementation_status"] = "verified"
        item["verified_reference_parity"] = False
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)

    def test_missing_evidence_and_missing_official_scope_fail_closed(self):
        changed = copy.deepcopy(self.data)
        changed["capabilities"][0]["repository_evidence"] = "../outside"
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)
        changed = copy.deepcopy(self.data)
        changed["capabilities"][0]["official_reference"] = "https://not-autodesk.example/"
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)
        changed = copy.deepcopy(self.data)
        changed["capabilities"] = [
            row for row in changed["capabilities"] if row["area"] != "toolsets"
        ]
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)

    def test_duplicate_feature_and_empty_acceptance_fail_closed(self):
        changed = copy.deepcopy(self.data)
        changed["capabilities"][1]["id"] = changed["capabilities"][0]["id"]
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)
        changed = copy.deepcopy(self.data)
        changed["capabilities"][0]["reference_equivalence_test"] = "run"
        with self.assertRaises(ValueError):
            validate_matrix(changed, ROOT)


if __name__ == "__main__":
    unittest.main()
