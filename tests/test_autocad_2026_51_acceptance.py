"""Regression tests: none of the 51 families may receive a fabricated pass."""
import copy
import json
import unittest

from autocad_2026_51_acceptance import (LEDGER, MATRIX, GATES,
                                         validate_ledger, safe_artifact)

class IndependentAcceptance51Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def test_all_51_families_mapped_once_and_not_falsely_approved(self):
        summary = validate_ledger(self.matrix, self.ledger)
        self.assertEqual(summary["total"], 51)
        self.assertEqual(summary["approved"], 0)
        self.assertEqual(len(set(x["id"] for x in self.ledger["cases"])), 51)
        self.assertEqual(set(GATES), set(self.ledger["cases"][0]["gates"]))

    def test_missing_or_duplicate_family_rejected(self):
        for change in ("missing", "duplicate", "unknown"):
            data = copy.deepcopy(self.ledger)
            if change == "missing":
                data["cases"].pop()
            if change == "duplicate":
                data["cases"][1]["id"] = data["cases"][0]["id"]
            if change == "unknown":
                data["cases"][0]["id"] = "invented_family"
            with self.subTest(change=change), self.assertRaises(ValueError):
                validate_ledger(self.matrix, data)

    def test_missing_gate_and_official_reference_mismatch_rejected(self):
        for change in ("gate", "reference", "criterion"):
            data = copy.deepcopy(self.ledger)
            entry = data["cases"][0]
            if change == "gate":
                entry["gates"].pop("android_physical")
            if change == "reference":
                entry["official_reference"] = "https://example.test"
            if change == "criterion":
                entry["reference_equivalence_test"] = "just works"
            with self.subTest(change=change), self.assertRaises(ValueError):
                validate_ledger(self.matrix, data)

    def test_fake_pass_without_artifact_or_hash_rejected(self):
        for gate in GATES:
            data = copy.deepcopy(self.ledger)
            data["cases"][0]["gates"][gate]["status"] = "passed"
            with self.subTest(gate=gate), self.assertRaises(ValueError):
                validate_ledger(self.matrix, data)

    def test_existing_code_not_reference_evidence(self):
        data = copy.deepcopy(self.ledger)
        row = data["cases"][0]
        row["gates"]["autocad_2026_reference"] = {
            "status":"passed",
            "artifact":"app/src/main/java/com/musa/cad/NativeDwg.java",
            "sha256":"a"*64}
        with self.assertRaises(ValueError):
            validate_ledger(self.matrix, data)

    def test_false_approval_cannot_modify_matrix_either(self):
        data = copy.deepcopy(self.ledger)
        data["cases"][0]["review"]["decision"] = "approved"
        with self.assertRaises(ValueError):
            validate_ledger(self.matrix, data)
        copy_matrix = copy.deepcopy(self.matrix)
        copy_matrix["capabilities"][0]["verified_reference_parity"] = True
        copy_matrix["capabilities"][0]["implementation_status"] = "verified"
        with self.assertRaises(ValueError):
            validate_ledger(copy_matrix, self.ledger)

    def test_scenario_inventory_must_be_real_and_nonempty(self):
        for change in ("missing", "duplicate", "incomplete"):
            data = copy.deepcopy(self.ledger)
            case = data["cases"][0]
            if change == "missing":
                case["scenarios"] = []
            if change == "duplicate":
                case["scenarios"].append(copy.deepcopy(case["scenarios"][0]))
            if change == "incomplete":
                case["scenario_inventory_complete"] = "yes"
            with self.subTest(change=change), self.assertRaises(ValueError):
                validate_ledger(self.matrix, data)

    def test_private_paths_and_external_urls_cannot_fake_local_artifacts(self):
        for path in ("../../secrets.json", "https://example.test/report.json",
                     "docs/autocad2026_evidence/../../README.md"):
            with self.subTest(path=path), self.assertRaises(ValueError):
                safe_artifact(path, "0"*64)

    def test_failed_gate_cannot_masquerade_as_completed(self):
        data = copy.deepcopy(self.ledger)
        row = data["cases"][0]["gates"]["semantic_comparison"]
        row.update({"status":"failed","artifact":"docs/autocad2026_evidence/fake.json","sha256":"0"*64})
        with self.assertRaises(ValueError):
            validate_ledger(self.matrix, data)

if __name__ == "__main__":
    unittest.main()
