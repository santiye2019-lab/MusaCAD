#!/usr/bin/env python3
"""Development progress accounting; final-release gate FAILS on any gap.

Do not mistake these planning points for measured AutoCAD parity. This
tool never fabricates CI conclusions or produces a production artifact.
"""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "CAD_2026_PROGRESS.json"
SHA40 = re.compile(r"^[0-9a-f]{40}$")


def assess(data, *, release=False, root=ROOT):
    if not isinstance(data, dict) or data.get("schema_version") != 1:
        raise ValueError("missing or unknown progress schema")
    base = data.get("baseline_points")
    if type(base) is not int or not 0 <= base <= 100:
        raise ValueError("invalid development baseline")
    done = data.get("completed_milestones")
    todo = data.get("backlog")
    modules = data.get("required_modules")
    if not isinstance(done, list) or not isinstance(todo, list) or not isinstance(modules, list):
        raise ValueError("progress inventory lists missing")
    if not done or not todo or not modules:
        raise ValueError("empty progress inventory cannot prove completion")
    names = set()
    credited = 0
    remaining = 0
    for row, is_done in [(r, True) for r in done] + [(r, False) for r in todo]:
        if not isinstance(row, dict) or not isinstance(row.get("id"), str) or not row["id"]:
            raise ValueError("invalid milestone")
        if row["id"] in names:
            raise ValueError("duplicate milestone")
        names.add(row["id"])
        points = row.get("points")
        if type(points) is not int or points < 1:
            raise ValueError("invalid milestone points")
        if is_done:
            if row.get("status") != "implemented":
                raise ValueError("unimplemented milestone credited")
            if not SHA40.fullmatch(str(row.get("commit", ""))):
                raise ValueError("credited milestone missing commit evidence")
            path = row.get("source_path")
            if (not isinstance(path, str) or Path(path).is_absolute() or ".." in Path(path).parts
                    or not (root / path).is_file()):
                raise ValueError("credited milestone missing source file")
            credited += points
        else:
            if row.get("status") not in ("in_progress", "not_started"):
                raise ValueError("failed or incorrectly marked backlog milestone")
            if not row.get("evidence_required"):
                raise ValueError("backlog lacks acceptance criteria")
            remaining += points
    if base + credited + remaining != 100:
        raise ValueError("planned points must total exactly 100")
    if any(type(x) is not dict or not x.get("id") for x in modules):
        raise ValueError("invalid required module")
    if len({x["id"] for x in modules}) != len(modules):
        raise ValueError("duplicate required module")
    for mod in modules:
        if mod.get("status") not in ("in_progress", "not_started", "verified"):
            raise ValueError("unknown or failing required module")
    acceptance = data.get("acceptance", {})
    if not isinstance(acceptance, dict):
        raise ValueError("invalid acceptance")
    for key in ("final_device_tests_total", "final_device_tests_passed",
                "failed_checks", "skipped_checks"):
        value = acceptance.get(key)
        if type(value) is not int or value < 0:
            raise ValueError("invalid acceptance counter: " + key)
    if acceptance["final_device_tests_passed"] > acceptance["final_device_tests_total"]:
        raise ValueError("more passes than planned tests")
    if release:
        errors = []
        if remaining:
            errors.append("remaining unimplemented milestone points")
        if any(mod["status"] != "verified" for mod in modules):
            errors.append("required modules are not all verified")
        if acceptance.get("full_ci") != "passed":
            errors.append("full CI is not verified green")
        if acceptance["final_device_tests_total"] < 18 or (
                acceptance["final_device_tests_passed"] != acceptance["final_device_tests_total"]):
            errors.append("final device acceptance not complete")
        if acceptance["failed_checks"] or acceptance["skipped_checks"]:
            errors.append("failed/skipped checks exist")
        if acceptance.get("independent_dwg_parity") != "passed":
            errors.append("original DWG independent roundtrip proof missing")
        if acceptance.get("release_approved") is not True:
            errors.append("release not explicitly approved")
        if errors:
            raise ValueError("release blocked: " + "; ".join(errors))
    return {"implementation_points": base + credited, "remaining_points": remaining,
            "required_modules": len(modules), "verified_modules":
                sum(m["status"] == "verified" for m in modules),
            "field_tests_passed": acceptance["final_device_tests_passed"],
            "field_tests_total": acceptance["final_device_tests_total"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--release", action="store_true",
                        help="Require complete implementation and all final acceptance")
    args = parser.parse_args()
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    result = assess(data, release=args.release)
    print("MusaCAD development estimate: {implementation_points}% | "
          "remaining {remaining_points}% | verified modules "
          "{verified_modules}/{required_modules} | final device checks "
          "{field_tests_passed}/{field_tests_total}".format(**result))
    if not args.release:
        print("Development inventory is valid; incomplete modules are NOT release-approved.")


if __name__ == "__main__":
    main()
