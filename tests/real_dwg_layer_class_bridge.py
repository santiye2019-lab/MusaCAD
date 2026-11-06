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
        "fixed_placeholder_ot80_identity_confirmed": False,
        "fixed_ltype_ot57_identity_confirmed": False,
        "terminal_fragment_2_bit_samples": 0,
        "terminal_fragment_4_bit_samples": 0,
        "terminal_fragment_all_zero_samples": 0,
        "terminal_fragment_nonzero_samples": 0,
        "terminal_fragment_undecoded_samples": 0,
        "terminal_fragment_2bit_pattern_mask": 0,
        "terminal_fragment_4bit_pattern_mask": 0,
        "terminal_fragment_start_alignment_mask": 0,
        "terminal_fragment_end_alignment_mask": 0,
        "h_tail_fully_decoded": False,
        "verified_raw_target_records": 0,
        "first_party_native_type_triplets_verified": 0,
        "first_party_native_byte_alignment_witnesses": 0,
        "first_party_source_local_raw_state_joins": 0,
        "raw_state_frozen_bit_samples": 0,
        "raw_state_on_bit_samples": 0,
        "raw_state_locked_bit_samples": 0,
        "raw_state_unmapped_bit_samples": 0,
        "raw_flags_are_not_visibility_or_color_parity": True,
        "byte_alignment_supports_padding_hypothesis_only": True,
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

        # This narrow pilot requires 3 CRC/self-H-backed absolute targets
        # for EACH sampled original LAYER. Extra targets/missing ordinals are
        # not silently promoted into fixed-type role evidence.
        if (targets != 3*n or raw80 != n or raw505 != n or raw57 != n or
                _integer(h, "sampled_layer_eed_terminal_candidate_attempts", 2048) != n or
                _integer(h, "sampled_layer_eed_terminal_candidate_absolute", 8192) != 3*n or
                _integer(h, "sampled_layer_eed_terminal_candidate_unmatched", 8192) != 0):
            raise EvidenceError("inconsistent_crc_backed_target_census")

        # The strict H parser currently rejects the final 2 or 4 bits.
        # Report that refusal even where all the bits are zero: zero is not
        # proof of padding and cannot authorize appearance/render semantics.
        fragment_verified = _integer(
            h, "sampled_layer_eed_terminal_fragment_verified", 2048)
        fragment_attempts = _integer(
            h, "sampled_layer_eed_terminal_fragment_attempts", 2048)
        fragment_two = _integer(
            h, "sampled_layer_eed_terminal_fragment_bits_2", 2048)
        fragment_four = _integer(
            h, "sampled_layer_eed_terminal_fragment_bits_4", 2048)
        fragment_zero = _integer(
            h, "sampled_layer_eed_terminal_fragment_zero", 2048)
        fragment_nonzero = _integer(
            h, "sampled_layer_eed_terminal_fragment_nonzero", 2048)
        complete_prefix = _integer(
            h, "sampled_layer_eed_terminal_fragment_prefix_four_tokens", 2048)
        if (fragment_verified != n or fragment_attempts != n or
                complete_prefix != n or fragment_two + fragment_four != n or
                fragment_zero + fragment_nonzero != n):
            raise EvidenceError("fragment_witness_not_closed")
        aggregate["terminal_fragment_2_bit_samples"] += fragment_two
        aggregate["terminal_fragment_4_bit_samples"] += fragment_four
        aggregate["terminal_fragment_all_zero_samples"] += fragment_zero
        aggregate["terminal_fragment_nonzero_samples"] += fragment_nonzero
        aggregate["terminal_fragment_undecoded_samples"] += n
        # OR masks of 2-bit/4-bit suffix values. The number of set mask bits
        # represents DISTINCT patterns, never their frequency or CAD role.
        pattern2 = _integer(
            h, "sampled_layer_eed_terminal_fragment_2bit_pattern_mask", 15)
        pattern4 = _integer(
            h, "sampled_layer_eed_terminal_fragment_4bit_pattern_mask", 65535)
        start_align = _integer(
            h, "sampled_layer_eed_terminal_fragment_start_mod8_mask", 255)
        end_align = _integer(
            h, "sampled_layer_eed_terminal_fragment_end_mod8_mask", 255)
        if (bool(pattern2) != bool(fragment_two) or
                bool(pattern4) != bool(fragment_four) or
                not start_align or not end_align or
                pattern2.bit_count() > fragment_two or
                pattern4.bit_count() > fragment_four or
                (fragment_zero == 0 and
                 ((pattern2 & 1) or (pattern4 & 1))) or
                (fragment_nonzero and fragment_two + fragment_four == n
                 and pattern2 <= 1 and pattern4 <= 1)):
            raise EvidenceError("impossible_terminal_bit_pattern_mask")
        # Every 2-bit fragment starts 2 bits before its end; every 4-bit
        # fragment starts 4 bits before its end, including bit-boundary wrap.
        implied_starts = 0
        for end_bit in range(8):
            if end_align & (1 << end_bit):
                if fragment_two:
                    implied_starts |= 1 << ((end_bit - 2) & 7)
                if fragment_four:
                    implied_starts |= 1 << ((end_bit - 4) & 7)
        if not start_align & implied_starts:
            raise EvidenceError("inconsistent_terminal_modulo_alignment")
        aggregate["terminal_fragment_2bit_pattern_mask"] |= pattern2
        aggregate["terminal_fragment_4bit_pattern_mask"] |= pattern4
        aggregate["terminal_fragment_start_alignment_mask"] |= start_align
        aggregate["terminal_fragment_end_alignment_mask"] |= end_align
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
        native_attempts = _integer(
            h, "sampled_layer_eed_terminal_triplet_attempts", 2048)
        native_verified = _integer(
            h, "sampled_layer_eed_terminal_triplet_verified", 2048)
        native_nonzero = _integer(
            h, "sampled_layer_eed_terminal_triplet_nonzero_fragment", 2048)
        if (native_attempts != n or native_verified > n or
                native_nonzero > native_verified or
                native_nonzero > fragment_nonzero):
            raise EvidenceError("contradictory_native_type_triplet_census")
        if not strict:
            aggregate["layer_samples_with_unresolved_target_class"] += n
            continue
        if native_verified != n or native_nonzero != fragment_nonzero:
            raise EvidenceError("native_type_triplet_not_proven")
        alignment_attempts = _integer(
            h, "sampled_layer_eed_terminal_alignment_attempts", 2048)
        alignment_verified = _integer(
            h, "sampled_layer_eed_terminal_alignment_verified", 2048)
        alignment_nonzero = _integer(
            h, "sampled_layer_eed_terminal_alignment_nonzero", 2048)
        if (alignment_attempts != n or alignment_verified != n or
                alignment_nonzero != fragment_nonzero):
            raise EvidenceError("original_native_byte_alignment_not_proven")
        raw_join_attempts = _integer(
            h, "sampled_layer_eed_state_join_attempts", 2048)
        raw_join_verified = _integer(
            h, "sampled_layer_eed_state_join_verified", 2048)
        raw_join_frozen = _integer(
            h, "sampled_layer_eed_state_join_frozen", 2048)
        raw_join_on = _integer(
            h, "sampled_layer_eed_state_join_on", 2048)
        raw_join_locked = _integer(
            h, "sampled_layer_eed_state_join_locked", 2048)
        raw_join_unmapped = _integer(
            h, "sampled_layer_eed_state_join_unmapped", 2048)
        if (raw_join_attempts != n or raw_join_verified != n or
                max(raw_join_frozen, raw_join_on, raw_join_locked,
                    raw_join_unmapped) > raw_join_verified or
                raw_join_verified > _integer(
                    h, "sampled_layer_eed_flags_verified", 2048) or
                raw_join_frozen > _integer(
                    h, "sampled_layer_eed_flags_frozen_bit", 2048) or
                raw_join_on > _integer(
                    h, "sampled_layer_eed_flags_on_bit", 2048) or
                raw_join_locked > _integer(
                    h, "sampled_layer_eed_flags_locked_bit", 2048) or
                raw_join_unmapped > _integer(
                    h, "sampled_layer_eed_flags_unmapped_bits", 2048)):
            raise EvidenceError("unproven_original_raw_flags_state_join")


        aggregate["layer_samples_with_three_crc_backed_targets"] += n
        aggregate["raw_ot80_target_records"] += n
        aggregate["raw_ot505_target_records"] += n
        aggregate["raw_ot57_target_records"] += n
        aggregate["verified_raw_target_records"] += 3*n
        aggregate["first_party_native_type_triplets_verified"] += n
        aggregate["first_party_native_byte_alignment_witnesses"] += n
        aggregate["first_party_source_local_raw_state_joins"] += n
        aggregate["raw_state_frozen_bit_samples"] += raw_join_frozen
        aggregate["raw_state_on_bit_samples"] += raw_join_on
        aggregate["raw_state_locked_bit_samples"] += raw_join_locked
        aggregate["raw_state_unmapped_bit_samples"] += raw_join_unmapped
        # Fixed OT identities are from the portable first-party DWG type
        # registry: OT80=PLACEHOLDER, OT57=LTYPE. Those NAMES are proven;
        # the meaning of their position within the OT51 H tail is not.
        aggregate["fixed_placeholder_ot80_identity_confirmed"] = True
        aggregate["fixed_ltype_ot57_identity_confirmed"] = True

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
    aggregate["terminal_fragment_2bit_pattern_values_seen"] = [
        i for i in range(4)
        if aggregate["terminal_fragment_2bit_pattern_mask"] & (1 << i)
    ]
    aggregate["terminal_fragment_4bit_pattern_values_seen"] = [
        i for i in range(16)
        if aggregate["terminal_fragment_4bit_pattern_mask"] & (1 << i)
    ]
    return aggregate


def require_minimum_evidence(report: dict, min_triplets: int, min_drawings: int) -> None:
    """Fail CI if its known real-source corpus silently loses coverage.

    Explicit CLI thresholds are pinned to an independently checked baseline.
    Programmatic summaries remain usable on smaller synthetic fixtures.
    """
    if type(min_triplets) is not int or type(min_drawings) is not int or (
        min_triplets < 0 or min_drawings < 0
    ):
        raise EvidenceError("bad_coverage_threshold")
    if (report["first_party_native_type_triplets_verified"] < min_triplets
            or report["drawings_with_local_material_class_join"] < min_drawings
            or report["layer_samples_with_three_crc_backed_targets"]
            < min_triplets):
        raise EvidenceError("original_real_dwg_corpus_coverage_regressed")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage-report", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--min-native-triplets", type=int, default=0)
    parser.add_argument("--min-source-drawings", type=int, default=0)
    args = parser.parse_args()
    source = json.loads(args.stage_report.read_text(encoding="utf-8"))
    result = summarize(source)
    require_minimum_evidence(result, args.min_native_triplets,
                             args.min_source_drawings)
    args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                           encoding="utf-8")
    print("AC1024 OT51 CRC target samples=%d, local MATERIAL class-identity joins=%d, "
          "visibility/render parity NOT TESTED" %
          (result["layer_samples_with_three_crc_backed_targets"],
           result["layer_samples_local_ot505_material_class_proven"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
