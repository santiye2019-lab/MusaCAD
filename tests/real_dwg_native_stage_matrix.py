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
    "ac1018_native_scene_open",
    "ac1018_record_ranges_and_crcs",
    "ac1024_page_map_header",
    "ac1024_physical_page_locations",
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
        # AC1018 index-only success is no longer enough to declare a
        # successful real-file stage. Every indexed record must pass the
        # native C11 CRC/frame and overlap audit before acceptance.
        if (result.get("signature") == "AC1018"
                and result["native_stage"] != "ac1018_record_ranges_and_crcs"):
            raise ValueError("ac1018_index_only_is_not_full_record_acceptance")
        if (result.get("signature") == "AC1024"
                and result["native_stage"] != "ac1024_physical_page_locations"):
            raise ValueError("ac1024_header_only_is_not_map_acceptance")
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
            if version in ("AC1015", "AC1018", "AC1024", "AC1032"):
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
