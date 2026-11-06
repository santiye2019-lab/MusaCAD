#!/usr/bin/env python3
"""Read-only diagnostics of each real DWG rejected by the native stage gate.

Only draws aggregate structural metadata from the existing first-party core.
Fails if source bytes change or if the diagnostic tool crashes. This report
does not infer valid geometry, object ownership, layers, or display fidelity.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

from real_dwg_lfs_corpus_audit import file_digest


def run() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=Path, default=Path("."))
    parser.add_argument("--stage-report", type=Path, required=True)
    parser.add_argument("--diagnostic", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()

    root = args.repository.resolve()
    stage = json.loads(args.stage_report.read_text(encoding="utf-8"))
    executable = args.diagnostic.resolve()
    entries = []
    failures = 0
    for item in stage["files"]:
        if item.get("result") != "rejected":
            continue
        relative = item["path"]
        original = root / relative
        row = {"path": relative, "source_stage": item["native_stage"]}
        entries.append(row)
        try:
            if original.is_symlink() or not original.is_file():
                raise ValueError("missing_or_symlinked_file")
            before = file_digest(original)
            if before[0] != item.get("committed_sha256"):
                raise ValueError("source_sha_changed_since_stage_report")
            result = subprocess.run(
                [str(executable), str(original)], capture_output=True,
                text=True, timeout=45, check=True,
            )
            triage = json.loads(result.stdout)
            after = file_digest(original)
            if before != after:
                raise ValueError("source_bytes_changed_during_diagnostic")
            if triage.get("geometry_verified") is not False:
                raise ValueError("unverified_geometry_claim")
            if triage.get("mutates_source") is not False:
                raise ValueError("source_mutation_claim")
            if triage.get("version") != item.get("signature"):
                raise ValueError("native_diagnostic_signature_disagreement")
            if triage.get("native_locations_ok") is True and (
                item.get("native_stage") ==
                "ac1032_physical_page_locations"
            ):
                raise ValueError("diagnostic_contradicts_native_stage")
            if (triage.get("version") == "AC1032" and
                    triage.get("map_lz_ok") is True and
                    triage.get("page_id_namespace_audit_complete") is True):
                counts = ("page_id_duplicate_positive",
                          "page_id_duplicate_gap",
                          "page_id_cross_role_collisions",
                          "page_ids_above_admission_cap")
                if any(type(triage.get(k)) is not int or triage[k] < 0
                       for k in counts):
                    raise ValueError("invalid_page_id_namespace_counters")
                if sum(triage[k] for k in counts) > triage.get(
                        "map_entry_count", 0):
                    raise ValueError("impossible_page_id_namespace_counters")
            row.update(triage)
            row["source_unchanged"] = True
        except (OSError, ValueError, subprocess.SubprocessError,
                json.JSONDecodeError) as exc:
            row["triage_error"] = type(exc).__name__ + ": " + str(exc)[:200]
            failures += 1

    report = {
        "scope": "rejected real DWGs: first-party physical map diagnostics only",
        "rejected_files_examined": len(entries),
        "diagnostic_failures": failures,
        "no_geometry_or_render_acceptance": True,
        "files": entries,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print("Real DWG rejection triage: %d checked; %d diagnostic failures" %
          (len(entries), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(run())
