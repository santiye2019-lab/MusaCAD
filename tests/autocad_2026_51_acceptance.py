#!/usr/bin/env python3
"""Conservative, evidence-driven 51-family AutoCAD 2026 acceptance ledger.

Neither a code presence check nor a passing synthetic unit test certifies
AutoCAD equivalence. Certified references must originate from real licensed
AutoCAD 2026 use; provenance needs independent human confirmation, which CI
alone CANNOT establish.
"""
from __future__ import annotations
import hashlib
import json
import pathlib
import re
from datetime import datetime

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs" / "AUTOCAD_2026_ACCEPTANCE_51.json"
MATRIX = ROOT / "docs" / "AUTOCAD_2026_PARITY_MATRIX.json"
EVIDENCE_PREFIX = "docs/autocad2026_evidence/"
GATES = (
    "autocad_2026_reference", "musacad_same_input", "semantic_comparison",
    "independent_dwg_roundtrip", "android_physical", "windows_desktop",
    "ios_physical", "continuous_integration",
)
STATES = {"not_run", "passed", "failed", "blocked"}
HEX64 = re.compile(r"^[a-fA-F0-9]{64}$")
TIMESTAMP = re.compile(r"^20[0-9]{2}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}")
SOURCE_TOOLS = {
    "autocad_2026_reference": "AutoCAD 2026",
    "musacad_same_input": "MusaCAD",
}


def safe_artifact(path: str, checksum: str, root=ROOT):
    """Verify committed evidence bytes, not an ungrounded label or URL."""
    if not isinstance(path, str) or not path.startswith(EVIDENCE_PREFIX):
        raise ValueError("Evidence must live in the versioned redacted evidence folder")
    relative = pathlib.PurePosixPath(path)
    if relative.is_absolute() or ".." in relative.parts or "\\\\" in path:
        raise ValueError("Unsafe artifact path")
    if not isinstance(checksum, str) or not HEX64.fullmatch(checksum):
        raise ValueError("Evidence needs a SHA-256 digest")
    full = root.joinpath(*relative.parts)
    if not full.is_file() or full.is_symlink():
        raise ValueError("Missing evidence artifact")
    data = full.read_bytes()
    if not data or hashlib.sha256(data).hexdigest() != checksum.lower():
        raise ValueError("Modified, empty or incorrectly hashed evidence")
    return data


def checked_iso(value):
    if not isinstance(value, str) or not TIMESTAMP.match(value):
        raise ValueError("Evidence needs an ISO timestamp")
    try:
        datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise ValueError("Invalid evidence timestamp") from exc


def validate_passed_gate(entry, gate, record, root=ROOT):
    raw = safe_artifact(record.get("artifact"), record.get("sha256"), root)
    try:
        content = json.loads(raw)
    except (ValueError, UnicodeError) as exc:
        raise ValueError("Evidence must be a machine-readable JSON report") from exc
    if not isinstance(content, dict) or content.get("schema_version") != 1:
        raise ValueError("Evidence report schema invalid")
    if (content.get("case_id") != entry["id"] or content.get("gate") != gate
            or content.get("result") != "passed"):
        raise ValueError("Incorrect report/case/gate or unsuccessful comparison")
    checked_iso(content.get("executed_at"))
    tool = content.get("tool")
    if not isinstance(tool, str) or len(tool.strip()) < 3:
        raise ValueError("Missing evidence-producing tool")
    if not isinstance(content.get("version"), str) or not content["version"].strip():
        raise ValueError("Missing tool version")
    if gate in SOURCE_TOOLS and tool != SOURCE_TOOLS[gate]:
        raise ValueError("Reference/candidate produced by wrong tool")
    if gate == "semantic_comparison":
        if content.get("difference_count") != 0:
            raise ValueError("Geometric/semantic differences not resolved")
        r = entry["gates"]["autocad_2026_reference"].get("sha256")
        m = entry["gates"]["musacad_same_input"].get("sha256")
        if (not r or not m or content.get("autocad_reference_sha256") != r
                or content.get("musacad_output_sha256") != m):
            raise ValueError("Comparison is not bound to the actual reference files")
        if content.get("input_sha256") in (None, ""):
            raise ValueError("Reference must identify the same input fixture")
    if gate == "independent_dwg_roundtrip":
        if (content.get("independent_reader") in (None, "", "MusaCAD")
                or content.get("object_semantic_equal") is not True
                or not content.get("source_dwg_sha256")
                or not HEX64.fullmatch(str(content.get("source_dwg_sha256")))):
            raise ValueError("Independent real DWG reopen proof required")
    if gate in ("android_physical", "ios_physical", "windows_desktop"):
        platform = gate.split("_", 1)[0]
        if content.get("platform") != platform or content.get("real_hardware") is not True:
            raise ValueError("Missing actual platform/device acceptance")
    if gate == "continuous_integration":
        url = content.get("run_url", "")
        if (not isinstance(url, str) or not url.startswith(
                "https://github.com/santiye2019-lab/MusaCAD/actions/runs/")
                or content.get("conclusion") != "success"
                or not re.fullmatch(r"[0-9a-f]{40}", str(content.get("commit_sha", "")))):
            raise ValueError("CI must link to successful actual workflow and commit")
    return content


def validate_ledger(matrix, ledger, root=ROOT):
    """Reject missing families, made-up approvals and unverifiable staged PASS."""
    if ledger.get("schema_version") != 1 or ledger.get("product") != "AutoCAD 2026":
        raise ValueError("Wrong 2026 ledger version or product")
    cases = ledger.get("cases")
    expected = matrix.get("capabilities")
    if not isinstance(cases, list) or not isinstance(expected, list):
        raise ValueError("Ledger/matrix cases missing")
    expected_by_id = {row["id"]: row for row in expected}
    if len(expected_by_id) != len(expected) or len(cases) != len(expected):
        raise ValueError("Ledger/matrix must have a unique 1:1 family mapping")
    if ledger.get("required_gates") != list(GATES):
        raise ValueError("Independent acceptance gates cannot be weakened")
    ids = set()
    passed = 0
    for entry in cases:
        if not isinstance(entry, dict) or not isinstance(entry.get("id"), str):
            raise ValueError("Invalid acceptance case")
        key = entry["id"]
        if key in ids or key not in expected_by_id:
            raise ValueError("Missing, repeated or unknown family ID")
        ids.add(key)
        source = expected_by_id[key]
        for field in ("area", "capability", "official_reference", "reference_equivalence_test"):
            if entry.get(field) != source[field]:
                raise ValueError("Acceptance scope diverged from the official matrix")
        complete = entry.get("scenario_inventory_complete")
        if type(complete) is not bool:
            raise ValueError("Scenario inventory completeness must be explicit")
        scenarios = entry.get("scenarios")
        if not isinstance(scenarios, list) or not scenarios:
            raise ValueError("Missing independent AutoCAD scenario")
        scenario_ids = set()
        for scenario in scenarios:
            if (not isinstance(scenario, dict) or
                    scenario.get("id") in scenario_ids or
                    not isinstance(scenario.get("id"), str) or
                    not scenario["id"].startswith(key + "-") or
                    not isinstance(scenario.get("procedure"), str) or
                    len(scenario["procedure"].strip()) < 20 or
                    scenario.get("status") not in STATES):
                raise ValueError("Incomplete or duplicate scenario")
            scenario_ids.add(scenario["id"])
        checks = entry.get("gates")
        if not isinstance(checks, dict) or set(checks) != set(GATES):
            raise ValueError("One of the eight mandatory gates is missing")
        observed = {}
        for gate in GATES:
            record = checks[gate]
            if not isinstance(record, dict) or record.get("status") not in STATES:
                raise ValueError("Unknown gate status")
            state = record["status"]
            observed[gate] = state
            if state == "passed":
                validate_passed_gate(entry, gate, record, root)
            elif record.get("artifact") is not None or record.get("sha256") is not None:
                raise ValueError("Unpassed gate must not imply proven artifact")
        review = entry.get("review")
        if not isinstance(review, dict) or review.get("decision") not in (
                "pending", "approved", "rejected"):
            raise ValueError("Missing decision")
        approved = review["decision"] == "approved"
        matrix_approved = source.get("verified_reference_parity") is True
        if approved != matrix_approved or (
                approved != (source.get("implementation_status") == "verified")):
            raise ValueError("Ledger and matrix verified flags must agree")
        if approved:
            if (not complete or any(s["status"] != "passed" for s in scenarios)
                    or any(observed[g] != "passed" for g in GATES)):
                raise ValueError("Full scenario and gate coverage required for approval")
            if (not isinstance(review.get("independent_reviewer"), str)
                    or len(review["independent_reviewer"].strip()) < 5):
                raise ValueError("Independent reviewer signoff required")
            checked_iso(review.get("reviewed_at"))
            attestation = safe_artifact(
                review.get("attestation_path"), review.get("attestation_sha256"), root)
            try:
                review_data = json.loads(attestation)
            except (ValueError, UnicodeError) as exc:
                raise ValueError("Reviewer attestation must be JSON") from exc
            if (review_data.get("case_id") != key
                    or review_data.get("decision") != "approved"
                    or review_data.get("independent_reviewer") != review["independent_reviewer"]):
                raise ValueError("Independently reviewed approval record mismatch")
            passed += 1
        elif (review.get("attestation_path") is not None
              or review.get("attestation_sha256") is not None):
            raise ValueError("Unapproved case must not claim a signed acceptance record")
    if ids != set(expected_by_id):
        raise ValueError("51-family ledger is incomplete")
    return {"total": len(expected), "approved": passed,
            "awaiting_independent_approval": len(expected) - passed}


def main():
    matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
    r = validate_ledger(matrix, ledger)
    print("AutoCAD 2026 capability-family independent acceptance: "
          "{approved}/{total} approved; {awaiting_independent_approval} pending. "
          "51-family starter matrix is NOT the entire AutoCAD 2026 command list."
          .format(**r))
    for entry in ledger["cases"]:
        if entry["review"]["decision"] != "approved":
            missing = ",".join(k for k, v in entry["gates"].items()
                               if v["status"] != "passed")
            print("{}: awaiting [{}]".format(entry["id"], missing))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
