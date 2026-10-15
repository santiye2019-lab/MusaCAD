#!/usr/bin/env python3
"""Fail-closed bookkeeping for AutoCAD 2026 reference equivalence.

This is NOT an AutoCAD emulator, an Autodesk test harness, or a source of
newly claimed equivalence. It rejects score inflation from code presence,
historical project planning points and synthetic-only tests.
"""
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MATRIX = ROOT / "docs" / "AUTOCAD_2026_PARITY_MATRIX.json"
COMMANDS = ROOT / "docs" / "AUTOCAD_2026_COMMAND_CATALOG.json"
STATES = {"partial", "blocked", "unassessed", "verified"}
DOMAINS = {
    "file", "draw", "annotation", "blocks", "documents", "3d",
    "automation", "collaboration", "geo", "toolsets",
}


def validate_matrix(data, root=ROOT):
    if data.get("schema_version") != 1 or data.get("scoring_policy", {}).get("complete_catalog") is not False:
        raise ValueError("Command-level reference catalogue has not been completed")
    if data.get("scoring_policy", {}).get("parity_percentage") is not None:
        raise ValueError("No parity percentage before exhaustive reference catalogue")
    if "AutoCAD 2026" not in data.get("target", ""):
        raise ValueError("Wrong parity reference")
    caps = data.get("capabilities")
    if not isinstance(caps, list) or len(caps) < 45:
        raise ValueError("Expected broad 2026 reference capability families")
    ids = set()
    statuses = {name: 0 for name in STATES}
    for entry in caps:
        if not isinstance(entry, dict):
            raise ValueError("Malformed capability record")
        identifier = entry.get("id")
        if (not isinstance(identifier, str) or not identifier or identifier in ids):
            raise ValueError("Duplicate or missing capability ID")
        ids.add(identifier)
        if entry.get("area") not in DOMAINS:
            raise ValueError("Unknown AutoCAD capability domain")
        if not isinstance(entry.get("capability"), str) or not entry["capability"].strip():
            raise ValueError("Missing capability title")
        if (not isinstance(entry.get("official_reference"), str) or
                not (entry["official_reference"].startswith("https://www.autodesk.com/") or
                     entry["official_reference"].startswith("https://help.autodesk.com/"))):
            raise ValueError("Reference must come from Autodesk documentation")
        acceptance = entry.get("reference_equivalence_test")
        if not isinstance(acceptance, str) or len(acceptance) < 30:
            raise ValueError("Missing independent AutoCAD behavior acceptance criterion")
        status = entry.get("implementation_status")
        if status not in STATES:
            raise ValueError("Unrecognized implementation status")
        statuses[status] += 1
        path = entry.get("repository_evidence")
        if path is not None:
            if (not isinstance(path, str) or pathlib.PurePosixPath(path).is_absolute()
                    or ".." in pathlib.PurePosixPath(path).parts
                    or not (root / path).is_file()):
                raise ValueError("Nonexistent/unsafe repository evidence")
        if status == "partial" and path is None:
            raise ValueError("Partial implementation needs traceable source")
        declared_verified = entry.get("verified_reference_parity")
        if declared_verified is not (status == "verified"):
            raise ValueError("Implementation state must agree with independent parity flag")
        proof = entry.get("parity_test_evidence")
        if status == "verified":
            if not isinstance(proof, dict):
                raise ValueError("Full parity cannot be claimed without evidence bundle")
            requirements = ("real_autocad_2026_result", "independent_real_dwg_cases",
                            "device_acceptance_cases", "regression_run")
            if any(not isinstance(proof.get(key), str) or
                   not proof[key].strip() for key in requirements):
                raise ValueError("All independent reference and device evidence required")
        elif proof is not None:
            raise ValueError("Nonverified feature cannot claim parity evidence")
    if "toolsets" not in {entry["area"] for entry in caps}:
        raise ValueError("Bundled specialist toolsets omitted from full AutoCAD target")
    return statuses



def validate_command_catalog(data):
    """Validate the first official 2026 command slice, without implying coverage."""
    if (data.get("schema_version") != 1 or data.get("inventory_complete") is not False
            or data.get("parity_percentage") is not None):
        raise ValueError("Reference command index is incomplete; parity must remain unknown")
    url = "https://help.autodesk.com/view/ACD/2026/ENU/?page=commands&q=*"
    if data.get("source_url") != url or "AutoCAD 2026" not in data.get("product", ""):
        raise ValueError("2026 source command index is missing or inconsistent")
    records = data.get("commands")
    if not isinstance(records, list) or len(records) < 200:
        raise ValueError("First 2026 command reference batch incomplete")
    valid_sections = {
        "command_line_variants", "three_d", "letter_a", "letter_b", "letter_c_start"
    }
    known_must_include = {
        "+CONSTRAINTSETTINGS", "-3DOSNAP", "-PDFIMPORT", "-MARKUPASSIST",
        "3DORBIT", "ACISIN", "ACISOUT", "ARX", "ACTRECORD", "ARRAYPATH",
        "AUTOCONSTRAIN", "BDETECT", "BSEARCH", "BCPARAMETER", "BEDIT",
        "BVSTATE", "BOX", "CHAMFER", "CIRCLE"
    }
    seen = set()
    result = {"unassessed": 0, "partial": 0, "blocked": 0, "verified": 0}
    for item in records:
        if not isinstance(item, dict):
            raise ValueError("Malformed official command entry")
        name = item.get("name")
        if (not isinstance(name, str) or not name or len(name) > 100
                or name != name.upper() or
                any(char not in "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-+?" for char in name)
                or name in seen):
            raise ValueError("Invalid or repeated Autodesk command name")
        seen.add(name)
        section = item.get("source_section")
        if section not in valid_sections:
            raise ValueError("Unknown official catalogue slice")
        if section == "command_line_variants" and not name.startswith(("-", "+")):
            raise ValueError("Prefixed variant lacks command-line prefix")
        if section == "three_d" and not name.startswith("3D"):
            raise ValueError("3D command grouped incorrectly")
        if section.startswith("letter_") and name[0] != section[7].upper():
            raise ValueError("Alphabetical command letter does not match section")
        if item.get("type") not in ("command", "express_tool"):
            raise ValueError("Invalid Autodesk command classification")
        eq = item.get("musa_equivalence_status")
        if eq not in result:
            raise ValueError("Unknown MusaCAD command-equivalence status")
        result[eq] += 1
        reference = item.get("reference_test_status")
        if reference not in ("not_run", "passed", "failed"):
            raise ValueError("Invalid AutoCAD reference test status")
        for key in ("autocad_reference_cases", "independent_reopen_cases",
                    "device_test_cases"):
            value = item.get(key)
            if (not isinstance(value, list) or any(
                    not isinstance(case, str) or not case.strip() for case in value)):
                raise ValueError("Reference and device evidence must be lists of named cases")
        evidence = item.get("independent_parity_evidence")
        if eq == "verified" or reference == "passed":
            if eq != "verified" or reference != "passed":
                raise ValueError("Reference-only test success is not complete parity")
            if any(not item[key] for key in
                   ("autocad_reference_cases", "independent_reopen_cases",
                    "device_test_cases")):
                raise ValueError("Real AutoCAD, independent DWG and device cases required")
            if not isinstance(evidence, dict) or not isinstance(
                    evidence.get("ci_run_url"), str
            ) or not evidence["ci_run_url"].startswith(
                    "https://github.com/santiye2019-lab/MusaCAD/actions/runs/"
            ):
                raise ValueError("Actual independent CI evidence required")
        elif evidence is not None:
            raise ValueError("Unverified command cannot carry claimed parity evidence")
    if not known_must_include.issubset(seen):
        raise ValueError("Official 2026 essential command slice incomplete")
    return result


def main():
    data = json.loads(MATRIX.read_text(encoding="utf-8"))
    statuses = validate_matrix(data)
    command_statuses = validate_command_catalog(json.loads(COMMANDS.read_text(encoding="utf-8")))
    total = sum(statuses.values())
    print(
        "AutoCAD 2026 starter inventory: %d capability families; "
        "%d partial, %d blocked, %d unassessed, %d independently verified."
        % (total, statuses["partial"], statuses["blocked"],
           statuses["unassessed"], statuses["verified"])
    )
    print(
        "Indexed commands (starter slice): %d; source/mobility parity verified: %d. "
        "This is NOT a full AutoCAD command catalogue."
        % (sum(command_statuses.values()), command_statuses["verified"])
    )
    print(
        "ACTUAL AUTOCAD 2026 PARITY PERCENTAGE: NOT YET MEASURABLE. "
        "Do NOT substitute historic 61%% development planning points or "
        "code-presence counts. Do not promote without real Autodesk reference "
        "and real DWG/physical-device evidence."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
