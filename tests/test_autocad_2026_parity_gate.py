import copy
import json
import unittest
from pathlib import Path

from autocad_2026_parity_gate import (MATRIX, COMMANDS, ROOT,
                                    validate_matrix, validate_command_catalog)


class AutoCAD2026ParityGateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MATRIX.read_text(encoding="utf-8"))

    def test_official_command_slice_has_no_fake_completed_commands(self):
        source = json.loads(COMMANDS.read_text(encoding="utf-8"))
        stats = validate_command_catalog(source)
        self.assertGreaterEqual(sum(stats.values()), 200)
        self.assertEqual(stats["verified"], 0)
        self.assertIsNone(source["parity_percentage"])
        self.assertFalse(source["inventory_complete"])
        self.assertIn("3DORBIT", {item["name"] for item in source["commands"]})
        self.assertIn("-PDFIMPORT", {item["name"] for item in source["commands"]})

    def test_partial_index_cannot_yield_a_parity_percentage(self):
        changed = json.loads(COMMANDS.read_text(encoding="utf-8"))
        changed["parity_percentage"] = 61
        with self.assertRaises(ValueError):
            validate_command_catalog(changed)
        changed["parity_percentage"] = None
        changed["inventory_complete"] = True
        with self.assertRaises(ValueError):
            validate_command_catalog(changed)

    def test_alias_names_and_command_entries_are_not_parity_proof(self):
        original = json.loads(COMMANDS.read_text(encoding="utf-8"))
        changed = copy.deepcopy(original)
        item = changed["commands"][0]
        item["musa_equivalence_status"] = "verified"
        with self.assertRaises(ValueError):
            validate_command_catalog(changed)
        item["reference_test_status"] = "passed"
        with self.assertRaises(ValueError):
            validate_command_catalog(changed)
        item["autocad_reference_cases"] = ["real AutoCAD draw case"]
        item["independent_reopen_cases"] = ["real source DWG fixture"]
        with self.assertRaises(ValueError):
            validate_command_catalog(changed)

    def test_official_index_command_integrity(self):
        original = json.loads(COMMANDS.read_text(encoding="utf-8"))
        cases = [
            lambda d: d["commands"][1].update(name=d["commands"][0]["name"]),
            lambda d: d["commands"][0].update(source_section="three_d"),
            lambda d: d["commands"][0].update(name="invented bad command"),
            lambda d: d["commands"][0].update(type="unsupported"),
            lambda d: d["commands"][0].update(reference_test_status="passed"),
            lambda d: d["commands"][0].update(independent_parity_evidence={"ci_run_url":"fake"}),
            lambda d: d["commands"][0].update(autocad_reference_cases=[""]),
            lambda d: d.update(source_url="https://example.com/fake")
        ]
        for mutate in cases:
            changed = copy.deepcopy(original)
            mutate(changed)
            with self.assertRaises(ValueError):
                validate_command_catalog(changed)

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
