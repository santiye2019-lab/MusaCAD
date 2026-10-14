import copy
import json
import unittest
from pathlib import Path

from cad_progress_gate import MANIFEST, ROOT, assess


class CadProgressGateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_development_percent_increments_and_unfinished_remains(self):
        result = assess(self.data)
        self.assertEqual(result["implementation_points"], 39)
        self.assertEqual(result["remaining_points"], 61)
        self.assertEqual(result["verified_modules"], 0)
        self.assertEqual(result["field_tests_passed"], 0)

    def test_final_apk_refused_while_any_module_is_unverified(self):
        with self.assertRaisesRegex(ValueError, "release blocked"):
            assess(self.data, release=True)

    def test_failed_module_or_spurious_points_refuse_even_development_check(self):
        bad = copy.deepcopy(self.data)
        bad["required_modules"][0]["status"] = "failed"
        with self.assertRaises(ValueError):
            assess(bad)
        bad = copy.deepcopy(self.data)
        bad["completed_milestones"][0]["points"] = 2
        with self.assertRaisesRegex(ValueError, "total exactly 100"):
            assess(bad)
        bad = copy.deepcopy(self.data)
        bad["completed_milestones"][0]["source_path"] = "../fake.java"
        with self.assertRaises(ValueError):
            assess(bad)

    def test_cannot_approve_release_just_by_setting_final_approval(self):
        bad = copy.deepcopy(self.data)
        bad["acceptance"]["release_approved"] = True
        with self.assertRaises(ValueError):
            assess(bad, release=True)


if __name__ == "__main__":
    unittest.main()
