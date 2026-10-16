"""Regression tests: none of the 51 families may receive a fabricated pass."""
import copy
import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from autocad_2026_51_acceptance import (LEDGER, MATRIX, GATES,
                                         validate_ledger, validate_passed_gate,
                                         safe_artifact)

class IndependentAcceptance51Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def test_all_51_families_mapped_once_and_not_falsely_approved(self):
        summary = validate_ledger(self.matrix, self.ledger)
        self.assertEqual(summary["total"], 51)
        self.assertEqual(summary["approved"], sum(
            row["review"]["decision"] == "approved"
            for row in self.ledger["cases"]))
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

    def test_cross_app_comparison_actually_checks_normalized_results(self):
        case = copy.deepcopy(self.ledger["cases"][0])
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            evidence = root / "docs" / "autocad2026_evidence"
            evidence.mkdir(parents=True)

            def stage(gate, tool, result):
                payload = {
                    "schema_version": 1, "case_id": case["id"],
                    "gate": gate, "result": "passed",
                    "tool": tool, "version": "2026" if tool == "AutoCAD 2026" else "1",
                    "executed_at": "2026-10-15T12:00:00Z",
                    "input_sha256": "a" * 64,
                    "normalized_result": result,
                }
                binary = (json.dumps(payload, sort_keys=True) + chr(10)).encode()
                path = "docs/autocad2026_evidence/" + gate + ".json"
                (evidence / (gate + ".json")).write_bytes(binary)
                return {"status":"passed", "artifact":path,
                        "sha256":hashlib.sha256(binary).hexdigest()}

            a = stage("autocad_2026_reference", "AutoCAD 2026", {"geometry": [1, 2]})
            m = stage("musacad_same_input", "MusaCAD", {"geometry": [1, 3]})
            case["gates"]["autocad_2026_reference"] = a
            case["gates"]["musacad_same_input"] = m
            comp = {
                "schema_version":1, "case_id":case["id"],
                "gate":"semantic_comparison", "result":"passed",
                "tool":"independent verifier", "version":"1",
                "executed_at":"2026-10-15T12:00:00Z",
                "input_sha256":"a"*64,
                "difference_count":0,
                "autocad_reference_sha256":a["sha256"],
                "musacad_output_sha256":m["sha256"],
            }
            encoded=(json.dumps(comp, sort_keys=True)+chr(10)).encode()
            (evidence / "semantic_comparison.json").write_bytes(encoded)
            row={"status":"passed",
                 "artifact":"docs/autocad2026_evidence/semantic_comparison.json",
                 "sha256":hashlib.sha256(encoded).hexdigest()}
            case["gates"]["semantic_comparison"] = row
            with self.assertRaisesRegex(ValueError, "semantic data differ"):
                validate_passed_gate(case, "semantic_comparison", row, root)

    def test_failed_gate_cannot_masquerade_as_completed(self):
        data = copy.deepcopy(self.ledger)
        row = data["cases"][0]["gates"]["semantic_comparison"]
        row.update({"status":"failed","artifact":"docs/autocad2026_evidence/fake.json","sha256":"0"*64})
        with self.assertRaises(ValueError):
            validate_ledger(self.matrix, data)

if __name__ == "__main__":
    unittest.main()
