#!/usr/bin/env python3
"""Report real-DWG first-party native read stages without claiming rendering.

LFS bytes are checked against the committed SHA-256 pointers *before and after*
each read-only native probe. A native rejection is recorded as a limitation,
not silently treated as a decoded drawing or an infrastructure failure.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
import sys

from real_dwg_lfs_corpus_audit import (
    LFS_POINTER, file_digest, git_bytes, tracked_dwgs,
)

KNOWN_STAGES = {
    "signature_only",
    "invalid_signature",
    "ac1015_section_locators",
    "ac1015_crc_verified_section_locators",
    "ac1018_page_map_header",
    "ac1018_physical_page_locations",
    "ac1018_objects_section_pages",
    "ac1018_handles_section_pages",
    "ac1018_crc_backed_handle_index",
    "ac1024_family_hypothesis_only",
    "ac1032_page_map_header",
    "ac1032_physical_page_locations",
    "ac1032_crc_backed_handle_index",
}
KNOWN_RESULTS = {"accepted", "rejected", "not_covered_by_this_native_probe"}


def run_probe(executable: Path, source: Path, timeout: float) -> dict:
    process = subprocess.run(
        [str(executable), str(source)], capture_output=True, text=True,
        timeout=timeout, check=False,
    )
    if process.returncode:
        raise RuntimeError("native_exit_%d" % process.returncode)
    result = json.loads(process.stdout)
    if (
        result.get("result") not in KNOWN_RESULTS
        or result.get("native_stage") not in KNOWN_STAGES
        or type(result.get("evidence_count")) is not int
        or result["evidence_count"] < 0
        or result.get("geometry_verified") is not False
        or result.get("rendering_verified") is not False
        or result.get("source_modified") is not False
    ):
        raise ValueError("invalid_native_probe_contract")
    if result["result"] == "accepted":
        if not result["native_stage"].startswith("ac") or not result["evidence_count"]:
            raise ValueError("unsupported_native_acceptance")
    hint = result.get("ac1024_compatibility_hypothesis")
    if result.get("signature") == "AC1024":
        if (
            result["result"] != "not_covered_by_this_native_probe"
            or result["native_stage"] != "ac1024_family_hypothesis_only"
            or result["evidence_count"] != 0
            or not isinstance(hint, dict)
            or hint.get("patched_signature_in_memory_only") is not True
            or hint.get("validated_original_ac1024_decoder") is not False
        ):
            raise ValueError("unsupported_ac1024_claim")
        keys = ("page_map_header", "physical_page_map",
                "objects_section_pages", "handles_section_pages",
                "candidate_handle_index")
        if any(type(hint.get(key)) is not bool for key in keys):
            raise ValueError("invalid_ac1024_hint")
        if any(hint[keys[i]] and not hint[keys[i - 1]]
               for i in range(1, len(keys))):
            raise ValueError("inconsistent_ac1024_hint")
        object_pages = hint.get("object_section_page_count")
        handle_pages = hint.get("handle_section_page_count")
        if (type(object_pages) is not int or type(handle_pages) is not int
                or object_pages < 0 or handle_pages < 0
                or object_pages > 65536 or handle_pages > 65536):
            raise ValueError("invalid_ac1024_section_page_counts")
        if hint["handles_section_pages"]:
            if not object_pages or not handle_pages:
                raise ValueError("missing_ac1024_section_page_counts")
        elif object_pages or handle_pages:
            raise ValueError("unverified_ac1024_section_page_counts")
        handles = hint.get("candidate_handle_count")
        if (type(handles) is not int or handles < 0 or handles > 1_000_000
                or bool(handles) != hint["candidate_handle_index"]):
            raise ValueError("invalid_ac1024_candidate_count")
        complete = hint.get("legacy_frame_probe_completed")
        if type(complete) is not bool:
            raise ValueError("invalid_ac1024_legacy_probe_state")
        fields = ("legacy_frame_crc_ok", "legacy_frame_crc_failed",
                  "legacy_raw_type_17_arc", "legacy_raw_type_18_circle",
                  "legacy_raw_type_19_line", "legacy_raw_type_77_lwpolyline")
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in fields):
            raise ValueError("invalid_ac1024_frame_count")
        if complete:
            if (not hint["candidate_handle_index"]
                    or handles > 400_000
                    or hint["legacy_frame_crc_ok"] +
                    hint["legacy_frame_crc_failed"] != handles):
                raise ValueError("invalid_ac1024_frame_coverage")
        elif any(hint[k] for k in fields):
            raise ValueError("unsupported_ac1024_raw_record_claim")
        if sum(hint[k] for k in fields[2:]) > hint["legacy_frame_crc_ok"]:
            raise ValueError("invalid_ac1024_type_claim")
        continuous = hint.get("continuous_offset_candidate_index")
        continuous_count = hint.get("continuous_candidate_handles")
        continuous_ok = hint.get("continuous_frame_crc_ok")
        continuous_bad = hint.get("continuous_frame_crc_failed")
        if (type(continuous) is not bool or
                any(type(value) is not int or value < 0
                    for value in (continuous_count, continuous_ok, continuous_bad))):
            raise ValueError("invalid_ac1024_continuous_contract")
        if continuous:
            if (not hint["candidate_handle_index"] or
                    continuous_count != handles or
                    continuous_count > 400_000 or
                    continuous_ok + continuous_bad != continuous_count):
                raise ValueError("invalid_ac1024_continuous_coverage")
        elif continuous_count or continuous_ok or continuous_bad:
            raise ValueError("unproven_ac1024_continuous_index")
        absolute = hint.get("page_absolute_candidate_index")
        absolute_count = hint.get("page_absolute_candidate_handles")
        absolute_ok = hint.get("page_absolute_frame_crc_ok")
        absolute_bad = hint.get("page_absolute_frame_crc_failed")
        if (type(absolute) is not bool or
                any(type(value) is not int or value < 0
                    for value in (absolute_count, absolute_ok, absolute_bad))):
            raise ValueError("invalid_ac1024_page_absolute_contract")
        if absolute:
            if (not hint["candidate_handle_index"] or
                    absolute_count != handles or
                    absolute_count > 400_000 or
                    absolute_ok + absolute_bad != absolute_count):
                raise ValueError("invalid_ac1024_page_absolute_coverage")
        elif absolute_count or absolute_ok or absolute_bad:
            raise ValueError("unproven_ac1024_page_absolute_index")
        measured = hint.get("legacy_offset_distribution_measured")
        offset_fields = ("legacy_distinct_offsets",
                         "legacy_duplicate_offset_entries",
                         "legacy_zero_offset_entries",
                         "legacy_max_offset_multiplicity",
                         "legacy_distinct_frame_crc_pass",
                         "legacy_repeated_crc_pass_entries")
        if (type(measured) is not bool or
                any(type(hint.get(k)) is not int or hint[k] < 0
                    for k in offset_fields)):
            raise ValueError("invalid_ac1024_offset_diagnostic_contract")
        (distinct, duplicates, zeros, max_repeat,
         distinct_crc, crc_entries) = (hint[k] for k in offset_fields)
        if measured:
            if (not hint["candidate_handle_index"] or
                    handles > 400_000 or
                    distinct + duplicates != handles or
                    not 1 <= distinct <= handles or
                    not 1 <= max_repeat <= handles or
                    zeros > max_repeat or
                    max_repeat > duplicates + 1 or
                    distinct_crc > distinct or
                    crc_entries > handles or
                    crc_entries < distinct_crc or
                    (distinct_crc == 0) != (crc_entries == 0)):
                raise ValueError("inconsistent_ac1024_offset_evidence")
            if duplicates == 0 and max_repeat != 1:
                raise ValueError("false_unique_ac1024_offset_claim")
        elif any(hint[k] for k in offset_fields):
            raise ValueError("unmeasured_ac1024_offset_evidence")
        status_fields = (
            "legacy_frame_too_short", "legacy_frame_invalid_ms",
            "legacy_frame_exceeds_section", "legacy_frame_crc_mismatch",
            "legacy_shift_minus_two_crc_pass",
            "legacy_shift_minus_one_crc_pass",
            "legacy_shift_plus_one_crc_pass",
            "legacy_shift_plus_two_crc_pass",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in status_fields):
            raise ValueError("invalid_ac1024_frame_reason_contract")
        samples = hint.get("legacy_shift_samples_tested")
        if type(samples) is not int or samples < 0 or samples > 2048:
            raise ValueError("invalid_ac1024_shift_sample_size")
        if measured:
            if (sum(hint[k] for k in status_fields[:4]) +
                    hint["legacy_repeated_crc_pass_entries"] != handles):
                raise ValueError("incomplete_ac1024_frame_reason_accounting")
            if not 1 <= samples <= distinct:
                raise ValueError("unrepresentative_ac1024_shift_sample")
            if any(hint[k] > samples for k in status_fields[4:]):
                raise ValueError("invalid_ac1024_shift_probe_count")
        elif samples or any(hint[k] for k in status_fields):
            raise ValueError("unmeasured_ac1024_frame_reason_claim")
        next_fields = (
            "legacy_next_offset_pairs",
            "legacy_next_frame_unavailable",
            "legacy_next_frame_exact",
            "legacy_next_frame_short",
            "legacy_next_frame_overlaps",
            "legacy_crc_ok_next_frame_exact",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in next_fields):
            raise ValueError("invalid_ac1024_next_frame_counts")
        (pairs, unavailable, exact, short, overlaps, crc_exact) = (
            hint[k] for k in next_fields)
        if measured:
            if (pairs != distinct - 1 or
                    unavailable + exact + short + overlaps != pairs or
                    crc_exact > exact or
                    crc_exact > distinct_crc or
                    unavailable > hint["legacy_frame_too_short"] +
                        hint["legacy_frame_invalid_ms"] +
                        hint["legacy_frame_exceeds_section"]):
                raise ValueError("inconsistent_ac1024_frame_adjacency")
        elif any(hint[k] for k in next_fields):
            raise ValueError("unmeasured_ac1024_frame_adjacency")
    elif hint is not None:
        raise ValueError("ac1024_hint_on_wrong_version")
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=Path, default=Path("."))
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--timeout-seconds", type=float, default=45.0)
    args = parser.parse_args()
    repo = args.repository.resolve()
    probe = args.probe.resolve()
    results = []
    states = Counter()
    versions = Counter()
    paths = tracked_dwgs(repo)
    for relative in paths:
        record: dict = {"path": relative}
        results.append(record)
        try:
            source = repo / relative
            if source.is_symlink() or not source.is_file():
                raise ValueError("source_missing_or_symlinked")
            committed = git_bytes(repo, "show", "HEAD:" + relative)
            pointer = LFS_POINTER.fullmatch(committed)
            if pointer is None:
                raise ValueError("missing_committed_lfs_pointer")
            expected_sha = pointer.group(1).decode("ascii")
            expected_size = int(pointer.group(2))
            actual_sha, actual_size, first = file_digest(source)
            if (actual_sha, actual_size) != (expected_sha, expected_size):
                raise ValueError("lfs_bytes_do_not_match_committed_pointer")
            observed = run_probe(probe, source, args.timeout_seconds)
            after_sha, after_size, after_first = file_digest(source)
            if (after_sha, after_size, after_first) != (
                actual_sha, actual_size, first
            ):
                raise ValueError("original_bytes_changed_during_probe")
            version = first.decode("ascii", errors="replace")
            if observed["signature"] != version:
                raise ValueError("native_signature_mismatch")
            # Fail closed on a result that mixes stages across DWG generations.
            stage = observed["native_stage"]
            if stage.startswith("ac") and not stage.startswith(version.lower()):
                raise ValueError("mismatched_native_format_stage")
            if version in ("AC1015", "AC1018", "AC1032"):
                if observed["result"] == "not_covered_by_this_native_probe":
                    raise ValueError("expected_native_probe_not_invoked")
            elif observed["result"] == "accepted":
                raise ValueError("unsupported_version_falsely_accepted")
            record.update(observed)
            record["committed_sha256"] = expected_sha
            record["size_bytes"] = actual_size
            record["source_unchanged"] = True
            states[observed["result"]] += 1
            versions[version] += 1
        except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError,
                subprocess.TimeoutExpired, json.JSONDecodeError) as exc:
            record["result"] = "audit_failure"
            record["error"] = type(exc).__name__ + ": " + str(exc)[:200]
            states["audit_failure"] += 1

    report = {
        "title": "MusaCAD real DWG native read-stage diagnostic, NOT geometry parity",
        "coverage": "original source LFS integrity + native C read-only subset",
        "no_geometry_layers_blocks_colors_writing_claim": True,
        "files_checked": len(paths),
        "result_counts": dict(sorted(states.items())),
        "version_counts": dict(sorted(versions.items())),
        "files": results,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print("Real DWG native-stage diagnostic: %s" % dict(sorted(states.items())))
    print("No unsupported or rejected file was accepted as rendered geometry.")
    return 0 if paths and not states["audit_failure"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
