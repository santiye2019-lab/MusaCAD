"""Fail-closed audit of granular AC1018 evidence; NO AutoCAD parity claims."""
import copy
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MACRO = ROOT / "docs/MUSACAD_AC1018_2D_INGEST_PROGRESS.json"
DETAIL = ROOT / "docs/MUSACAD_AC1018_2D_READ_DETAIL.json"


def assess_detail(macro, detail, root=ROOT):
    if detail.get("schema_version") != 1 or detail.get("autoCAD_2026_parity_percent") is not None:
        raise ValueError("schema or claimed AutoCAD parity")
    if detail.get("metric_kind") != "partial_acceptance_evidence_index_not_full_engine_or_autocad_parity":
        raise ValueError("wrong interpretation")
    if detail.get("source_macro_manifest") != "docs/MUSACAD_AC1018_2D_INGEST_PROGRESS.json":
        raise ValueError("wrong macro manifest")
    milestones = macro["milestones"]
    ids = [m["id"] for m in milestones]
    if len(ids) != len(set(ids)) or len(ids) != macro["total_milestones"]:
        raise ValueError("macro milestone inventory invalid")
    verified = sum(m["status"] == "verified" for m in milestones)
    if verified != macro["verified_milestones"] or any(
        m["status"] not in ("verified", "pending") for m in milestones
    ):
        raise ValueError("macro acceptance inconsistent")
    if macro["coverage_percent"] != 100.0 * verified / len(milestones):
        raise ValueError("macro percentage inflated")
    if detail["macro_total"] != len(milestones) or detail["macro_verified"] != verified:
        raise ValueError("macro checkpoint counts inconsistent")
    if detail["original_macro_coverage_percent"] != macro["coverage_percent"]:
        raise ValueError("original binary milestone progress altered")
    per = detail["subchecks_per_pending_macro"]
    if not isinstance(per, int) or per != 8:
        raise ValueError("fixed eight-item subcheck policy changed")
    groups = detail["groups"]
    pending = {m["id"] for m in milestones if m["status"] == "pending"}
    if len(groups) != len(pending) or {g["macro_milestone_id"] for g in groups} != pending:
        raise ValueError("pending macro-gate inventory changed or duplicated")
    sub_ids = set()
    evidenced = 0
    for group in groups:
        steps = group["steps"]
        if len(steps) != per:
            raise ValueError("missing or added subchecks without policy revision")
        for step in steps:
            sid = step["id"]
            if not sid or sid in sub_ids:
                raise ValueError("missing or repeated subcheck ID")
            sub_ids.add(sid)
            if step["status"] == "verified":
                evidenced += 1
                if not isinstance(step.get("merged_pr"), int) or step["merged_pr"] < 1:
                    raise ValueError("unattributed purportedly verified subcheck")
                files = step.get("evidence", [])
                if len(files) < 2 or not step.get("qualification"):
                    raise ValueError("verified entry lacks concrete evidence or limits")
                if not any(s.startswith("core/native/src/") for s in files) or not any(
                    s.startswith("core/native/tests/") for s in files
                ):
                    raise ValueError("missing portable core source or regression test")
                for rel in files:
                    path = Path(rel)
                    if path.is_absolute() or ".." in path.parts or not (root / path).is_file():
                        raise ValueError("unsafe or missing evidence path")
            elif step["status"] == "pending":
                if "merged_pr" in step or "evidence" in step:
                    raise ValueError("pending milestone improperly credited")
            else:
                raise ValueError("illegal subcheck status")
    total = len(milestones) * per
    earned = verified * per + evidenced
    if detail["verified_pending_subchecks"] != evidenced or (
        detail["detail_checks_verified"] != earned or detail["detail_checks_total"] != total
    ):
        raise ValueError("earned evidence count does not match statuses")
    if detail["detail_coverage_percent"] != earned * 100.0 / total:
        raise ValueError("fractional progress percentage artificially inflated")
    if earned > total or detail["detail_coverage_percent"] >= 100 and pending:
        raise ValueError("partially completed macro recorded as fully accepted")
    return {"macro_percent": macro["coverage_percent"],
            "evidence_percent": detail["detail_coverage_percent"],
            "evidenced_subchecks": evidenced, "earned": earned, "total": total}


class Ac1018ReadProgressGateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.macro = json.loads(MACRO.read_text(encoding="utf-8"))
        cls.detail = json.loads(DETAIL.read_text(encoding="utf-8"))

    def test_verified_subchecks_have_real_source_and_ci_merged_pr_evidence(self):
        result = assess_detail(self.macro, self.detail)
        self.assertEqual(result["macro_percent"], 68.75)
        self.assertEqual(result["evidence_percent"], 71.875)
        self.assertEqual(result["evidenced_subchecks"], 4)

    def test_arbitrary_evidence_credit_is_rejected(self):
        inflated = copy.deepcopy(self.detail)
        inflated["detail_coverage_percent"] = 90.0
        with self.assertRaisesRegex(ValueError, "inflated"):
            assess_detail(self.macro, inflated)
        inflated = copy.deepcopy(self.detail)
        inflated["verified_pending_subchecks"] += 1
        with self.assertRaisesRegex(ValueError, "earned evidence"):
            assess_detail(self.macro, inflated)

    def test_no_unverified_full_macro_credit(self):
        inflated = copy.deepcopy(self.macro)
        inflated["verified_milestones"] += 1
        with self.assertRaisesRegex(ValueError, "macro acceptance"):
            assess_detail(inflated, self.detail)

    def test_missing_test_file_or_undeclared_gate_rejected(self):
        inflated = copy.deepcopy(self.detail)
        inflated["groups"][0]["steps"][0]["evidence"][1] = "core/native/tests/missing_fake.c"
        with self.assertRaisesRegex(ValueError, "evidence path"):
            assess_detail(self.macro, inflated)
        inflated = copy.deepcopy(self.detail)
        inflated["groups"][0]["steps"][0]["status"] = "pending"
        with self.assertRaisesRegex(ValueError, "pending milestone"):
            assess_detail(self.macro, inflated)

    def test_cannot_claim_autocad_parity(self):
        inflated = copy.deepcopy(self.detail)
        inflated["autoCAD_2026_parity_percent"] = 71.875
        with self.assertRaisesRegex(ValueError, "claimed AutoCAD parity"):
            assess_detail(self.macro, inflated)


if __name__ == "__main__":
    unittest.main()
