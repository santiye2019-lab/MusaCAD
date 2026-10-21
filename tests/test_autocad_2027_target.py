"""Fail closed on invented AutoCAD 2027 equivalence or lost historical coverage."""
import copy
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LATEST = ROOT / "docs/AUTOCAD_2027_TARGET.json"
PREVIOUS = ROOT / "docs/AUTOCAD_2026_PARITY_MATRIX.json"
LEGACY_ACCEPTANCE = ROOT / "docs/AUTOCAD_2026_ACCEPTANCE_51.json"
DELTAS = {"geometry_cleanup", "checkout", "connected_references",
          "assistant_2027", "forma_data_management"}
PRIORITY = {"P0", "P1", "P2", "P3"}


def validate_target(target, matrix, legacy):
    if target.get("target") != "AutoCAD 2027":
        raise ValueError("Target must be AutoCAD 2027")
    if target.get("source_baseline", {}).get("previous_family_count") != 51:
        raise ValueError("Existing family denominator cannot change silently")
    if target.get("reporting", {}).get("family_denominator") != 51:
        raise ValueError("All 51 fixed families are mandatory")
    if target.get("reporting", {}).get("initial_certified_family_count") != 0:
        raise ValueError("A planning migration is NOT parity certification")
    if target.get("reporting", {}).get("independent_approval_required") is not True:
        raise ValueError("Independent approval cannot be skipped")
    native = target.get("native_dwg_compatibility", {})
    if "AC1032" not in native.get("external", "") or "MC1032" not in native.get("first_party_private_format", ""):
        raise ValueError("Native format must not be renamed to Autodesk DWG")
    policy = target.get("build_policy", {})
    if (policy.get("third_party_cad_decoder_runtime") is not False
            or policy.get("apk_without_user_final_approval") is not False
            or policy.get("unified_core") is not True
            or policy.get("fail_closed_on_unsupported") is not True
            or set(policy.get("platforms", [])) != {"Android", "Windows", "iOS"}):
        raise ValueError("Owner engine and deployment policy must remain strict")
    families = target.get("families")
    expected = matrix.get("capabilities")
    if not isinstance(families, list) or not isinstance(expected, list):
        raise ValueError("Family lists missing")
    if len(families) != 51 or len(expected) != 51 or len(legacy.get("cases", [])) != 51:
        raise ValueError("51 legacy capability families must remain complete")
    if [x["id"] for x in families] != [x["id"] for x in expected]:
        raise ValueError("2027 IDs/order diverge from 2026 history")
    if [x["id"] for x in families] != [x["id"] for x in legacy["cases"]]:
        raise ValueError("2027 target diverges from strict acceptance history")
    for index, (new, old) in enumerate(zip(families, expected), start=1):
        if (new.get("index") != index or new.get("area") != old["area"]
                or new.get("capability") != old["capability"]
                or new.get("legacy_2026_implementation_status") != old["implementation_status"]
                or new.get("priority") not in PRIORITY):
            raise ValueError("Historical family identity or priority corrupted")
        if (new.get("status_2027") != "not_certified"
                or new.get("independently_verified") is not False
                or new.get("scenario_inventory_complete") is not False):
            raise ValueError("Do not copy 2026 partial code evidence as 2027 certified")
    deltas = target.get("release_2027_deltas")
    if not isinstance(deltas, list) or {d.get("id") for d in deltas} != DELTAS or len(deltas) != 5:
        raise ValueError("All documented 2027 enhancements required")
    known = {c["id"] for c in families}
    for delta in deltas:
        if (delta.get("status") != "not_certified"
                or delta.get("implementation_status") != "not_assessed"
                or not delta.get("family_ids")
                or any(x not in known for x in delta["family_ids"])
                or not delta.get("reference_url", "").startswith(
                    ("https://www.autodesk.com/", "https://help.autodesk.com/"))):
            raise ValueError("Unsupported delta proof or cross-reference")
    return {"families": len(families), "enhancements": len(deltas),
            "certified_2027": 0}


class AutoCAD2027TargetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.target = json.loads(LATEST.read_text(encoding="utf-8"))
        cls.matrix = json.loads(PREVIOUS.read_text(encoding="utf-8"))
        cls.legacy = json.loads(LEGACY_ACCEPTANCE.read_text(encoding="utf-8"))

    def test_preserves_51_historical_ids_without_misrepresenting_completion(self):
        self.assertEqual(validate_target(self.target, self.matrix, self.legacy),
                         {"families": 51, "enhancements": 5, "certified_2027": 0})

    def test_fabricated_family_certification_rejected(self):
        target = copy.deepcopy(self.target)
        target["families"][0]["status_2027"] = "certified"
        with self.assertRaises(ValueError):
            validate_target(target, self.matrix, self.legacy)

    def test_missing_family_or_delta_ref_rejected(self):
        for corrupt in ("missing", "invented"):
            with self.subTest(corrupt=corrupt):
                target = copy.deepcopy(self.target)
                if corrupt == "missing":
                    target["families"].pop(25)
                else:
                    target["release_2027_deltas"][0]["family_ids"] = ["not_real"]
                with self.assertRaises(ValueError):
                    validate_target(target, self.matrix, self.legacy)

    def test_apk_release_and_third_party_core_policies_cannot_be_weakened(self):
        for field in ("apk_without_user_final_approval", "third_party_cad_decoder_runtime"):
            target = copy.deepcopy(self.target)
            target["build_policy"][field] = True
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_target(target, self.matrix, self.legacy)


if __name__ == "__main__":
    unittest.main()
