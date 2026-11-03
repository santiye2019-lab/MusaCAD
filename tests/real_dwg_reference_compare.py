#!/usr/bin/env python3
"""Isolated read-only LibreDWG observable oracle versus MusaCAD native stage.

Only public corpus files are used. No GPL source is imported or linked to MusaCAD.
A native *stage* is not a decoded entity: entity parity is explicitly NOT measured.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from real_dwg_lfs_corpus_audit import LFS_POINTER, file_digest, git_bytes, tracked_dwgs
from real_dwg_native_stage_matrix import run_probe

PILOT = "Projeler/01-KARAGÜMRÜK STADYUM - PİS SU TESİSATI.dwg"
SAFE_TYPE = re.compile(r"[A-Z0-9][A-Z0-9_]{0,63}\Z")


def observable_inventory(payload: object) -> dict:
    """Count only explicitly identified records in LibreDWG's OBJECTS array.

    Never infer an entity from its numeric 'type' field or count unrecognized
    record forms as decoded geometry. No handles, layer names or text leak.
    """
    if not isinstance(payload, dict) or not isinstance(payload.get("OBJECTS"), list):
        raise ValueError("unsupported_libredwg_json_schema")
    objects = payload["OBJECTS"]
    if len(objects) > 2_000_000:
        raise ValueError("unbounded_reference_objects")
    entity = Counter()
    other = Counter()
    unknown = 0
    for record in objects:
        if not isinstance(record, dict):
            unknown += 1
            continue
        # LibreDWG JSON distinguishes {"entity":"LINE"} and {"object":"LAYER"}.
        # Do not interpret 'type': usually a numeric internal DWG class number.
        kind = record.get("entity")
        if isinstance(kind, str) and SAFE_TYPE.fullmatch(kind):
            entity[kind] += 1
            continue
        kind = record.get("object")
        if isinstance(kind, str) and SAFE_TYPE.fullmatch(kind):
            other[kind] += 1
            continue
        unknown += 1
    if not entity and not other:
        raise ValueError("reference_has_no_typed_objects")
    return {
        "reference_total_object_records": len(objects),
        "reference_typed_entity_count": sum(entity.values()),
        "reference_typed_nonentity_count": sum(other.values()),
        "reference_unclassified_count": unknown,
        "reference_entities_by_type": dict(sorted(entity.items())),
        "reference_layer_record_count": other.get("LAYER", 0),
        "reference_block_header_record_count": other.get("BLOCK_HEADER", 0),
        "reference_nonentities_by_type": dict(sorted(other.items())),
        "reference_status": "libredwg_typed_inventory_only",
    }


def observable_inventory_stream(stream) -> dict:
    """Count typed OBJECTS in a potentially multi-gigabyte JSON stream.

    ijson emits parser events one at a time. In particular, non-OBJECTS
    payloads, geometry, coordinates, labels and handles are never materialized
    into a large Python object. A record contributes to the count only after
    its complete top-level object has been seen.
    """
    import ijson  # CI-only host tool; never linked or bundled into MusaCAD.

    entity = Counter()
    other = Counter()
    unknown = 0
    total = 0
    seen_objects = False
    in_record = False
    record_entity = None
    record_object = None
    decoder_warning_mask = None

    for prefix, event, value in ijson.parse(stream):
        if prefix == "DECODER_WARNING_MASK" and event == "number":
            decoder_warning_mask = int(value)
        if prefix == "OBJECTS" and event == "start_array":
            if seen_objects:
                raise ValueError("duplicate_reference_objects_array")
            seen_objects = True
        elif seen_objects and prefix == "OBJECTS.item":
            if event == "start_map":
                in_record = True
                record_entity = None
                record_object = None
            elif event == "end_map" and in_record:
                total += 1
                if record_entity is not None:
                    entity[record_entity] += 1
                elif record_object is not None:
                    other[record_object] += 1
                else:
                    unknown += 1
                in_record = False
            elif event in ("start_array", "string", "number", "boolean", "null"):
                # Non-map array entries are not decoded CAD entities.
                total += 1
                unknown += 1
        elif in_record and prefix == "OBJECTS.item.entity":
            if event == "string" and SAFE_TYPE.fullmatch(value):
                record_entity = value
            elif event != "map_key":
                record_entity = None
        elif in_record and prefix == "OBJECTS.item.object":
            if event == "string" and SAFE_TYPE.fullmatch(value):
                record_object = value
            elif event != "map_key":
                record_object = None
        if total > 2_000_000:
            raise ValueError("unbounded_reference_objects")

    if not seen_objects:
        raise ValueError("unsupported_libredwg_json_schema")
    if not entity and not other:
        raise ValueError("reference_has_no_typed_objects")
    result = {
        "reference_total_object_records": total,
        "reference_typed_entity_count": sum(entity.values()),
        "reference_typed_nonentity_count": sum(other.values()),
        "reference_unclassified_count": unknown,
        "reference_entities_by_type": dict(sorted(entity.items())),
        "reference_layer_record_count": other.get("LAYER", 0),
        "reference_block_header_record_count": other.get("BLOCK_HEADER", 0),
        "reference_nonentities_by_type": dict(sorted(other.items())),
        "reference_status": "libredwg_typed_inventory_only",
    }
    if decoder_warning_mask is not None:
        result["reference_decoder_warning_mask"] = decoder_warning_mask
    return result



def reference_read(executable: Path, source: Path, timeout: int, host_probe: Path | None = None) -> dict:
    """Try strict JSON, then LibreDWG's independently formatted minJSON.

    A successful exit code is never mistaken for successful parsing. Each
    candidate must be structurally valid and contain typed CAD object records.
    No raw output, drawing text, geometry or parser excerpts enter the report.
    """
    import ijson

    with tempfile.TemporaryDirectory(prefix="musacad-oracle-") as folder:
        output = Path(folder) / "reference.json"
        attempts = []
        for mode in ("JSON", "minJSON"):
            with output.open("wb") as stream:
                try:
                    process = subprocess.run(
                        [str(executable), "-O", mode, str(source)],
                        stdout=stream, stderr=subprocess.DEVNULL,
                        check=False, timeout=timeout,
                    )
                except subprocess.TimeoutExpired:
                    attempts.append({"mode": mode, "status": "timeout"})
                    continue
            if process.returncode != 0:
                attempts.append({"mode": mode, "status": "exit_nonzero",
                                 "exit_code": process.returncode})
                continue
            byte_count = output.stat().st_size
            try:
                with output.open("rb") as stream:
                    inventory = observable_inventory_stream(stream)
                inventory["reference_json_bytes"] = byte_count
                inventory["reference_output_mode"] = mode
                return inventory
            except (OSError, UnicodeError, json.JSONDecodeError,
                    ValueError, ijson.JSONError) as error:
                message = str(error).lower()
                if "premature eof" in message or "incomplete json" in message:
                    kind = "premature_eof"
                elif "lexical error" in message:
                    kind = "invalid_json_token"
                elif isinstance(error, ValueError) and str(error) in (
                    "unsupported_libredwg_json_schema",
                    "reference_has_no_typed_objects",
                    "unbounded_reference_objects",
                ):
                    kind = str(error)
                else:
                    kind = "invalid_json_structure"
                attempts.append({"mode": mode, "status": kind,
                                 "output_bytes": byte_count})

        if host_probe is not None:
            with output.open("wb") as stream:
                try:
                    process = subprocess.run(
                        [str(host_probe), str(source)],
                        stdout=stream, stderr=subprocess.DEVNULL,
                        check=False, timeout=timeout,
                    )
                except subprocess.TimeoutExpired:
                    attempts.append({"mode": "direct_types", "status": "timeout"})
                    process = None
            if process is not None and process.returncode == 0:
                try:
                    with output.open("rb") as stream:
                        inventory = observable_inventory_stream(stream)
                    inventory["reference_json_bytes"] = output.stat().st_size
                    inventory["reference_output_mode"] = "libredwg_host_direct_types"
                    inventory["reference_json_exporter_failed"] = True
                    return inventory
                except (OSError, ValueError, ijson.JSONError):
                    attempts.append({"mode": "direct_types",
                                     "status": "invalid_typed_inventory"})
            elif process is not None:
                attempts.append({"mode": "direct_types",
                                 "status": "exit_nonzero",
                                 "exit_code": process.returncode})

        if all(item["status"] == "exit_nonzero" for item in attempts):
            return {"reference_status": "libredwg_rejected_or_failed",
                    "reference_exit_code": attempts[0]["exit_code"],
                    "reference_attempts": attempts}
        if all(item["status"] == "timeout" for item in attempts):
            return {"reference_status": "libredwg_timeout",
                    "reference_attempts": attempts}
        return {"reference_status": "libredwg_unusable_json",
                "reference_attempts": attempts}


def ac1024_candidate_alignment(reference: dict, native: dict) -> dict:
    """Report count correlations, NEVER an AC1024 entity or drawing parity."""
    if (native.get("signature") != "AC1024" or
            reference.get("reference_status") != "libredwg_typed_inventory_only"):
        return {"state": "NOT_APPLICABLE", "entity_parity": "NOT_MEASURABLE"}
    hint = native.get("ac1024_compatibility_hypothesis", {})
    if not hint.get("candidate_handle_index"):
        return {"state": "FIRSTPARTY_CANDIDATE_UNAVAILABLE",
                "entity_parity": "NOT_MEASURABLE"}
    total = reference["reference_total_object_records"]
    count = hint["candidate_handle_count"]
    result = {
        "state": ("CANDIDATE_HANDLE_COUNT_EQUAL_NOT_ENTITY_PARITY"
                  if count == total else "CANDIDATE_HANDLE_COUNT_MISMATCH"),
        "reference_records": total,
        "firstparty_legacy_candidate_handles": count,
        "type_parity_verified": False,
        "layer_owner_color_parity_verified": False,
        "render_parity_verified": False,
        "entity_parity": "NOT_MEASURABLE",
    }
    if hint.get("legacy_frame_probe_completed"):
        raw = {
            "17": hint["legacy_raw_type_17_arc"],
            "18": hint["legacy_raw_type_18_circle"],
            "19": hint["legacy_raw_type_19_line"],
            "77": hint["legacy_raw_type_77_lwpolyline"],
        }
        result["legacy_frame_crc_verified_records"] = hint["legacy_frame_crc_ok"]
        result["legacy_frame_crc_rejected_records"] = hint["legacy_frame_crc_failed"]
        result["raw_numeric_type_codes_not_entity_names"] = raw
        # This only compares historical numeric associations as *hints*;
        # a matching count is not identity/class/owner or render parity.
        if hint["legacy_frame_crc_failed"] == 0:
            entities = reference["reference_entities_by_type"]
            labels = {"17": "ARC", "18": "CIRCLE",
                      "19": "LINE", "77": "LWPOLYLINE"}
            result["legacy_raw_code_count_correlations"] = {
                code: {
                    "legacy_raw_code_count": value,
                    "reference_named_entity_count": entities.get(name, 0),
                    "count_equal_only": value == entities.get(name, 0),
                } for code, value in raw.items()
                for name in (labels[code],)
            }
    if hint.get("mc_frame_probe_completed"):
        result["r2010_frame_crc_verified_records"] = hint["mc_frame_crc_pass"]
        result["r2010_frame_unreadable_records"] = hint["mc_frame_unreadable"]
        result["r2010_raw_ot_declared_records"] = hint["mc_ot_decoded"]
        # Raw OT declaration correlations are never object identity or
        # geometry proof. This isolated reference is not AutoCAD parity.
        ot_map = {
            "1": ("mc_ot_raw_1", "TEXT", "entity"),
            "7": ("mc_ot_raw_7", "INSERT", "entity"),
            "17": ("mc_ot_raw_17", "ARC", "entity"),
            "18": ("mc_ot_raw_18", "CIRCLE", "entity"),
            "19": ("mc_ot_raw_19", "LINE", "entity"),
            "44": ("mc_ot_raw_44", "MTEXT", "entity"),
            "49": ("mc_ot_raw_49", "BLOCK_HEADER", "object"),
            "51": ("mc_ot_raw_51", "LAYER", "object"),
            "77": ("mc_ot_raw_77", "LWPOLYLINE", "entity"),
        }
        entity_counts = reference.get("reference_entities_by_type", {})
        object_counts = reference.get("reference_nonentities_by_type", {})
        result["r2010_raw_ot_count_correlations"] = {
            code: {
                "firstparty_crc_backed_raw_ot_count": hint[key],
                "reference_type_count": (
                    entity_counts if kind == "entity" else object_counts
                ).get(name, 0),
                "counts_equal_only": hint[key] == (
                    entity_counts if kind == "entity" else object_counts
                ).get(name, 0),
                "verified_entity_identity": False,
            }
            for code, (key, name, kind) in ot_map.items()
        }
    return result



def audit_source(repo: Path, relative: str) -> tuple[Path, str, int]:
    source = repo / relative
    if not relative.startswith("Projeler/") or source.is_symlink() or not source.is_file():
        raise ValueError("not_a_tracked_public_corpus_file")
    pointer = LFS_POINTER.fullmatch(git_bytes(repo, "show", "HEAD:" + relative))
    if pointer is None:
        raise ValueError("missing_committed_lfs_pointer")
    expected_sha = pointer.group(1).decode("ascii")
    expected_size = int(pointer.group(2))
    actual_sha, actual_size, _ = file_digest(source)
    if (actual_sha, actual_size) != (expected_sha, expected_size):
        raise ValueError("git_lfs_source_unhydrated_or_changed")
    return source, expected_sha, actual_size


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=Path, default=Path("."))
    parser.add_argument("--dwgread", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--reference-probe", type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--timeout-seconds", type=int, default=90)
    args = parser.parse_args()
    repo = args.repository.resolve()
    executable = args.dwgread.resolve()
    probe = args.probe.resolve()
    host_probe = args.reference_probe.resolve() if args.reference_probe else None
    if (not executable.is_file() or not probe.is_file()
            or (host_probe is not None and not host_probe.is_file())):
        parser.error("host tools not found; this is infrastructure failure")
    tracked = tracked_dwgs(repo)
    paths = tracked if args.all else [PILOT]
    rows = []
    infra_failures = 0
    reference_failures = 0
    for relative in paths:
        row: dict = {
            "source": relative,
            "entity_parity": "NOT_MEASURABLE",
            "render_parity": "NOT_TESTED",
            "autocad_equivalence": "NOT_TESTED",
            "firstparty_entity_inventory": "NOT_AVAILABLE_FROM_STAGE_PROBE",
        }
        rows.append(row)
        try:
            if relative not in tracked:
                raise ValueError("missing_tracked_pilot")
            source, digest, byte_count = audit_source(repo, relative)
            row["source_sha256"] = digest
            row["source_bytes"] = byte_count
            row["libredwg"] = reference_read(executable, source, args.timeout_seconds, host_probe)
            row["musacad"] = run_probe(probe, source, float(args.timeout_seconds))
            row["ac1024_candidate_alignment"] = ac1024_candidate_alignment(
                row["libredwg"], row["musacad"])
            after_digest, after_size, _ = file_digest(source)
            if (after_digest, after_size) != (digest, byte_count):
                raise ValueError("source_was_modified")
            row["source_unchanged"] = True
            reference_observed = (row["libredwg"]["reference_status"]
                                  == "libredwg_typed_inventory_only")
            if not reference_observed:
                reference_failures += 1
            row["comparison_state"] = (
                "REFERENCE_OBSERVED_NATIVE_STAGE_ONLY_NO_ENTITY_PARITY"
                if reference_observed
                else "REFERENCE_UNAVAILABLE_NATIVE_STAGE_ONLY"
            )
        except (OSError, ValueError, subprocess.SubprocessError, json.JSONDecodeError) as error:
            infra_failures += 1
            row["comparison_state"] = "INFRASTRUCTURE_OR_INTEGRITY_FAILURE"
            # Do not copy tool stderr, drawing content or paths into diagnostics.
            row["error_code"] = type(error).__name__
            row["error_category"] = (
                str(error) if isinstance(error, ValueError)
                and str(error) in {"missing_tracked_pilot", "missing_committed_lfs_pointer",
                    "git_lfs_source_unhydrated_or_changed", "source_was_modified",
                    "not_a_tracked_public_corpus_file"}
                else "tool_or_report_contract_failed"
            )
    report = {
        "check": "host_only_libredwg_observable_reference_vs_firstparty_native_stage",
        "reference_tool": "upstream LibreDWG dwgread JSON/minJSON or isolated decoded-object host CLI",
        "firstparty_tool": "core/native MusaCAD stage probe (not a geometry exporter)",
        "uses_libredwg_in_musacad_runtime": False,
        "coverage": "reference type counts and independent native read stage only",
        "NOT_a_full_entity_comparison": True,
        "raw_drawings_in_report": False,
        "files_checked": len(rows),
        "infrastructure_failures": infra_failures,
        "reference_failures": reference_failures,
        "files": rows,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)
                           + "\n", encoding="utf-8")
    for row in rows:
        reference = row.get("libredwg", {}).get("reference_status", "not_run")
        stage = row.get("musacad", {}).get("native_stage", "not_run")
        print(f"{row['source']}: LibreDWG={reference}; MusaCAD={stage}; "
              f"entity_parity={row['entity_parity']}")
    print("WARNING: native stage success does not prove entity, layer, color, block or render parity.")
    return 1 if infra_failures or reference_failures or not rows else 0


if __name__ == "__main__":
    sys.exit(main())
