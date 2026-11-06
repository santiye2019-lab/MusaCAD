#!/usr/bin/env python3
"""Source-scoped OT51 handle-target / local MATERIAL-class evidence.

Read-only aggregate diagnostics. Do NOT treat an ordinal OT match as a DWG
material property, LAYER linetype, plotstyle, color or render acceptance.
OT 505 is drawing-local and must be independently resolved from that SAME
drawing's CRC-authenticated R2010 Classes section.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


class EvidenceError(ValueError):
    pass


def _integer(h: dict, key: str, bound: int = 10_000_000) -> int:
    value = h.get(key)
    if type(value) is not int or not 0 <= value <= bound:
        raise EvidenceError("bad_count:" + key)
    return value


def _flag(h: dict, key: str) -> bool:
    value = h.get(key)
    if type(value) is not bool:
        raise EvidenceError("bad_flag:" + key)
    return value


def summarize(stage: dict) -> dict:
    if not isinstance(stage, dict) or type(stage.get("files_checked")) is not int \
            or not isinstance(stage.get("files"), list) \
            or stage["files_checked"] != len(stage["files"]) \
            or stage.get("no_geometry_layers_blocks_colors_writing_claim") is not True:
        raise EvidenceError("bad_stage_provenance")

    aggregate = {
        "title": "MusaCAD source-scoped OT51 raw H references and local OT505 DXF evidence",
        "source_files_checked": len(stage["files"]),
        "ac1024_files_checked": 0,
        "layer_samples_with_three_crc_backed_targets": 0,
        "layer_samples_local_ot505_material_class_proven": 0,
        "layer_samples_with_unresolved_target_class": 0,
        "drawings_with_local_material_class_join": 0,
        "raw_ot80_target_records": 0,
        "raw_ot505_target_records": 0,
        "raw_ot57_target_records": 0,
        "verified_raw_target_records": 0,
        "material_property_link_semantics_proven": False,
        "ot80_semantics_proven": False,
        "ot57_semantics_proven": False,
        "layer_visibility_parity_proven": False,
        "rendering_verified": False,
        "source_modified": False,
    }
    for item in stage["files"]:
        if not isinstance(item, dict) or not isinstance(item.get("signature"), str):
            raise EvidenceError("bad_file")
        if item["signature"] != "AC1024":
            continue
        aggregate["ac1024_files_checked"] += 1
        if item.get("source_modified") is not False \
                or item.get("source_unchanged") is not True \
                or item.get("geometry_verified") is not False \
                or item.get("rendering_verified") is not False:
            raise EvidenceError("source_changed_or_false_render_claim")
        h = item.get("ac1024_compatibility_hypothesis")
        if not isinstance(h, dict):
            raise EvidenceError("missing_ac1024_provenance")

        n = _integer(h, "sampled_layer_eed_terminal_candidate_verified", 2048)
        raw80 = _integer(h, "sampled_layer_eed_terminal_candidate_ot80_placeholder", 8192)
        raw57 = _integer(h, "sampled_layer_eed_terminal_candidate_ot57_ltype", 8192)
        raw505 = _integer(h, "sampled_layer_eed_terminal_candidate_dynamic_class_unresolved", 8192)
        targets = _integer(h, "sampled_layer_eed_terminal_candidate_matched", 8192)
        # Other observed raw OTs are legal but cannot enter this narrow bridge.
        if not n:
            continue
        if not _flag(h, "source_classes_section_verified") \
                or not _flag(h, "source_classes_extracted") \
                or not _flag(h, "source_classes_r2010_numeric_verified"):
            raise EvidenceError("layer_target_without_authenticated_classes")

        if n > 2048 or targets < 3*n:
            raise EvidenceError("invalid_layer_target_total")
        ordinals = [
            _integer(h, "sampled_layer_eed_terminal_candidate_ordinal_%d_matched" % i, 2048)
            for i in (2, 3, 4)
        ]
        consensus = [
            _integer(h, "sampled_layer_eed_terminal_candidate_ordinal_%d_raw_ot_consensus" % i, 65535)
            for i in (2, 3, 4)
        ]
        # The source-stage observer reports raw OT+1 for each ordinal consensus.
        strict = (ordinals == [n, n, n]
                  and consensus == [81, 506, 58]
                  and raw80 >= n and raw505 >= n and raw57 >= n)
        if not strict:
            aggregate["layer_samples_with_unresolved_target_class"] += n
            continue

        aggregate["layer_samples_with_three_crc_backed_targets"] += n
        aggregate["raw_ot80_target_records"] += n
        aggregate["raw_ot505_target_records"] += n
        aggregate["raw_ot57_target_records"] += n
        aggregate["verified_raw_target_records"] += 3*n

        local_name = h.get("source_classes_r2010_505_dxf_identifier")
        local_name_valid = _flag(h, "source_classes_r2010_505_dxf_identifier_verified")
        local_class_present = _flag(h, "source_classes_r2010_ot505_present")
        object_class = _flag(h, "source_classes_r2010_ot505_object")
        tu_valid = _flag(h, "source_classes_r2010_tu_verified")
        if local_name_valid and local_name == "MATERIAL" \
                and local_class_present and object_class and tu_valid:
            aggregate["layer_samples_local_ot505_material_class_proven"] += n
            aggregate["drawings_with_local_material_class_join"] += 1
        else:
            aggregate["layer_samples_with_unresolved_target_class"] += n
    return aggregate


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage-report", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    source = json.loads(args.stage_report.read_text(encoding="utf-8"))
    result = summarize(source)
    args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                           encoding="utf-8")
    print("AC1024 OT51 CRC target samples=%d, local MATERIAL class-identity joins=%d, "
          "visibility/render parity NOT TESTED" %
          (result["layer_samples_with_three_crc_backed_targets"],
           result["layer_samples_local_ot505_material_class_proven"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
