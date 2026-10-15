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


def main():
    data = json.loads(MATRIX.read_text(encoding="utf-8"))
    statuses = validate_matrix(data)
    total = sum(statuses.values())
    print(
        "AutoCAD 2026 starter inventory: %d capability families; "
        "%d partial, %d blocked, %d unassessed, %d independently verified."
        % (total, statuses["partial"], statuses["blocked"],
           statuses["unassessed"], statuses["verified"])
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
