"""Machine-checkable conservative native MusaCAD core gate coverage.

Exactly 25 fixed acceptance gates. Candidate work does not increase score.
The score is NOT percent AutoCAD parity, app completion, or effort remaining.
"""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOC = ROOT / "docs" / "MUSACAD_CORE_COMPLETION_GATES.json"


class CoreCompletionGatesTest(unittest.TestCase):
    def test_fixed_scoring_and_no_false_parity(self):
        doc = json.loads(DOC.read_text(encoding="utf-8"))
        gates = doc["gates"]
        self.assertEqual(25, doc["total_gates"])
        self.assertEqual(25, len(gates))
        names = [gate["id"] for gate in gates]
        self.assertEqual(len(names), len(set(names)))
        self.assertTrue(all(gate["status"] in ("accepted", "candidate", "pending")
                            for gate in gates))
        accepted = [g for g in gates if g["status"] == "accepted"]
        self.assertEqual(len(accepted), doc["accepted_gates"])
        self.assertEqual(4 * len(accepted), doc["completion_index_percent"])
        self.assertIsNone(doc["autoCAD_2026_parity_percent"])
        self.assertEqual(0, doc["certified_51_families"])
        for gate in accepted:
            self.assertTrue(gate.get("evidence"))
            self.assertTrue((ROOT / gate["evidence"]).is_file(),
                            "Missing evidence file: " + str(gate["evidence"]))
        # The current common prefix parser must not count until all five
        # required workflow results were completed successfully.
        common = [g for g in gates if g["id"] == "common_prefix_subset"]
        self.assertEqual(1, len(common))
        self.assertIn(common[0]["status"], ("accepted", "candidate"))
        if common[0]["status"] == "candidate":
            self.assertLess(doc["completion_index_percent"], 40)

    def test_autoCAD_2026_never_inferred_from_successful_ci(self):
        audit = json.loads((ROOT / "docs" / "AUTOCAD_2026_ACCEPTANCE_51.json")
                           .read_text(encoding="utf-8"))
        cert = [case for case in audit["cases"]
                if case.get("review", {}).get("decision") == "approved"]
        gates = json.loads(DOC.read_text(encoding="utf-8"))
        self.assertEqual(len(cert), gates["certified_51_families"])


if __name__ == "__main__":
    unittest.main()
