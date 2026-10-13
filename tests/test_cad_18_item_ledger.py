"""Protect the canonical 18-item CAD engine inventory against accidental drift.

This does NOT assert features are complete; it guards the acceptance ledger's
existence, exact numbering, source traceability and explicit device gate.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs" / "CAD_ENGINE_18_ITEM_LEDGER.md"

class InventoryLedgerContract(unittest.TestCase):
    def test_all_original_items_present_once_and_in_order(self):
        body = LEDGER.read_text(encoding="utf-8")
        items = re.findall(r"^\\|\\s*([0-9]{2})\\s*\\|", body, flags=re.MULTILINE)
        self.assertEqual(items, [f"{n:02d}" for n in range(1, 19)])
        self.assertIn("github.com/santiye2019-lab/MusaCAD/issues/249", body)

    def test_no_unsupported_all_done_or_main_release_claim(self):
        body = LEDGER.read_text(encoding="utf-8")
        self.assertIn("18/18 madde nihai kabul bekliyor", body)
        self.assertIn("GitHub CI ≠ gerçek Android cihaz kabulü", body)
        self.assertIn("main", body)
        self.assertIn("ACIS", body)
        self.assertIn("AutoCAD", body)
        for marker in ("**F0", "**F1", "**F2", "**F3", "**F4", "**F5"):
            self.assertIn(marker, body, marker)
        rows = [line for line in body.splitlines() if
                re.match(r"^\\|\\s*[0-9]{2}\\s*\\|", line)]
        self.assertEqual(len(rows), 18)
        for row in rows:
            self.assertIn(" | ", row)
            self.assertNotIn("| TAMAMLANDI |", row,
                "A claim of complete requires fixture+CI+device evidence outside this static contract")

if __name__ == "__main__":
    unittest.main()
