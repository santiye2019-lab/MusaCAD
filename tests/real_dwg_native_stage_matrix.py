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
        original_keys = (
            "source_page_map_header", "source_physical_page_map",
            "source_patched_page_count_equal",
        )
        if any(type(hint.get(key)) is not bool for key in original_keys):
            raise ValueError("invalid_ac1024_original_page_map_type")
        original_count = hint.get("source_physical_page_count")
        if (type(original_count) is not int or not 0 <= original_count <= 131072):
            raise ValueError("invalid_ac1024_original_page_count")
        if hint["source_physical_page_map"] and (
            not hint["source_page_map_header"] or original_count == 0
        ):
            raise ValueError("unverified_ac1024_original_page_map")
        if not hint["source_physical_page_map"] and original_count:
            raise ValueError("unsupported_ac1024_original_page_count")
        if hint["source_patched_page_count_equal"] and (
            not hint["source_physical_page_map"]
            or not hint.get("physical_page_map")
        ):
            raise ValueError("unverified_ac1024_original_page_comparison")
        cls_bool = (
            "source_classes_section_verified",
            "source_classes_extracted",
            "source_classes_numeric_no_extra",
            "source_classes_numeric_with_extra",
        )
        cls_int = (
            "source_classes_page_count", "source_classes_logical_bytes",
            "source_classes_count_no_extra", "source_classes_count_with_extra",
        )
        if any(type(hint.get(k)) is not bool for k in cls_bool) or any(
            type(hint.get(k)) is not int or hint[k] < 0 for k in cls_int
        ):
            raise ValueError("invalid_ac1024_original_classes_type")
        if hint["source_classes_section_verified"]:
            if (not hint["source_physical_page_map"]
                    or not 1 <= hint["source_classes_page_count"] <= 65536
                    or not 16 <= hint["source_classes_logical_bytes"] <= 8 * 1024 * 1024):
                raise ValueError("invalid_ac1024_original_classes_pages")
        elif (hint["source_classes_extracted"]
              or hint["source_classes_page_count"]
              or hint["source_classes_logical_bytes"]):
            raise ValueError("unsupported_ac1024_original_classes_evidence")
        if hint["source_classes_extracted"] and not hint["source_classes_section_verified"]:
            raise ValueError("unverified_ac1024_original_classes_transfer")
        for present, count_key in (
            ("source_classes_numeric_no_extra", "source_classes_count_no_extra"),
            ("source_classes_numeric_with_extra", "source_classes_count_with_extra"),
        ):
            count = hint[count_key]
            if (hint[present] != (1 <= count <= 4096)
                    or (hint[present] and not hint["source_classes_extracted"])):
                raise ValueError("unsupported_ac1024_numeric_class_claim")
        for flag, bits, maximum in (
            ("source_classes_r2010_bitcount_no_extra",
             "source_classes_r2010_bits_no_extra",
             "source_classes_r2010_max_class_no_extra"),
            ("source_classes_r2010_bitcount_with_extra",
             "source_classes_r2010_bits_with_extra",
             "source_classes_r2010_max_class_with_extra"),
        ):
            if (type(hint.get(flag)) is not bool or
                    type(hint.get(bits)) is not int or
                    type(hint.get(maximum)) is not int):
                raise ValueError("invalid_ac1024_r2010_bitcount_type")
            if hint[flag]:
                if (not hint["source_classes_extracted"] or
                        not 36 <= hint[bits] <= 8 * hint["source_classes_logical_bytes"] or
                        not 500 <= hint[maximum] <= 5000):
                    raise ValueError("unverified_ac1024_r2010_class_prefix")
            elif hint[bits] or hint[maximum]:
                raise ValueError("unsupported_ac1024_r2010_class_prefix")
        for tail_field in (
            "source_classes_r2010_tail8_with_extra",
            "source_classes_r2010_extent_exact_with_extra",
        ):
            if type(hint.get(tail_field)) is not bool:
                raise ValueError("invalid_ac1024_class_tail_type")
            if hint[tail_field] and not hint["source_classes_r2010_bitcount_with_extra"]:
                raise ValueError("unverified_ac1024_class_tail_evidence")

        span_ok = hint.get("source_classes_r2010_string_span_bounded")
        span_bits = hint.get("source_classes_r2010_string_bits")
        span_wide = hint.get("source_classes_r2010_string_wide_length")
        if type(span_ok) is not bool or type(span_wide) is not bool or \
                type(span_bits) is not int:
            raise ValueError("invalid_ac1024_string_span_type")
        if span_ok:
            if (not hint["source_classes_section_verified"]
                    or not hint["source_classes_extracted"]
                    or not hint["source_classes_r2010_bitcount_with_extra"]
                    or not hint["source_classes_r2010_extent_exact_with_extra"]
                    or not 0 < span_bits <
                    hint["source_classes_r2010_bits_with_extra"] - 32):
                raise ValueError("unproven_ac1024_string_span")
        elif span_bits or span_wide:
            raise ValueError("unverified_ac1024_string_span_values")
        # A string bit-window does not prove valid TU strings, any class
        # name, or a shared class identity across two different drawings.

        tu_verified = hint.get("source_classes_r2010_tu_verified")
        tu_fields = hint.get("source_classes_r2010_tu_fields")
        tu_units = hint.get("source_classes_r2010_tu_units")
        tu_nonempty = hint.get("source_classes_r2010_tu_505_three_nonempty")
        tu_ascii = hint.get("source_classes_r2010_tu_505_ascii_only")
        if any(type(x) is not bool for x in (tu_verified,tu_nonempty,tu_ascii)):
            raise ValueError("invalid_ac1024_tu_audit_flags")
        if (type(tu_fields) is not int or type(tu_units) is not int
                or not 0 <= tu_fields <= 12288
                or not 0 <= tu_units <= 8 * 1024 * 1024):
            raise ValueError("invalid_ac1024_tu_audit_counts")
        if tu_verified:
            if (not span_ok or
                    not hint["source_classes_r2010_numeric_verified"] or
                    not hint["source_classes_r2010_ot505_present"] or
                    tu_fields != 3 * hint["source_classes_r2010_numeric_count"] or
                    tu_units < int(tu_nonempty) * 3 or
                    tu_units > span_bits // 16):
                raise ValueError("tu_audit_without_exact_source_provenance")
        elif tu_fields or tu_units or tu_nonempty or tu_ascii:
            raise ValueError("tu_audit_values_without_proof")
        # TU Unicode validity establishes bounded class-name fields only.
        # No class-name contents, class semantics, or draw acceptance is emitted.

        dxf_valid = hint.get("source_classes_r2010_505_dxf_identifier_verified")
        dxf_size = hint.get("source_classes_r2010_505_dxf_identifier_length")
        dxf_name = hint.get("source_classes_r2010_505_dxf_identifier")
        valid_name_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_:.-"
        if (type(dxf_valid) is not bool or type(dxf_size) is not int or
                not 0 <= dxf_size <= 96 or not isinstance(dxf_name, str)):
            raise ValueError("invalid_ac1024_class505_dxf_types")
        if dxf_valid:
            if (not tu_verified or not tu_nonempty or not tu_ascii or
                    not 1 <= len(dxf_name) == dxf_size <= 96 or
                    any(ch not in valid_name_chars for ch in dxf_name)):
                raise ValueError("unverified_ac1024_class505_dxf_identifier")
        elif dxf_size or dxf_name:
            raise ValueError("class505_dxf_identifier_without_proof")
        # This is only the third TU string of local class 505. Never infer
        # material, layer inheritance, writer or render admission from it.

        numeric_flags = (
            "source_classes_r2010_numeric_verified",
            "source_classes_r2010_ot505_present",
            "source_classes_r2010_ot505_entity",
            "source_classes_r2010_ot505_object",
        )
        if any(type(hint.get(k)) is not bool for k in numeric_flags):
            raise ValueError("invalid_ac1024_r2010_numeric_flag")
        numeric_count = hint.get("source_classes_r2010_numeric_count")
        if type(numeric_count) is not int or numeric_count < 0:
            raise ValueError("invalid_ac1024_r2010_numeric_count")
        if hint["source_classes_r2010_numeric_verified"]:
            if (not hint["source_classes_extracted"] or
                    not hint["source_classes_r2010_bitcount_with_extra"] or
                    not 1 <= numeric_count <= min(
                        4096,hint["source_classes_r2010_max_class_with_extra"]-499
                    )):
                raise ValueError("unsupported_ac1024_r2010_numeric_provenance")
        elif numeric_count:
            raise ValueError("unverified_ac1024_r2010_numeric_count")
        has505 = hint["source_classes_r2010_ot505_present"]
        entity = hint["source_classes_r2010_ot505_entity"]
        obj = hint["source_classes_r2010_ot505_object"]
        if has505:
            if (not hint["source_classes_r2010_numeric_verified"]
                    or not (entity ^ obj)):
                raise ValueError("unproven_ac1024_ot505_numeric_class")
        elif entity or obj:
            raise ValueError("unsupported_ac1024_ot505_kind")
        survey_fields = (
            "source_classes_survey_begin_at_zero",
            "source_classes_survey_end_at_eof",
            "source_classes_survey_begin_in_first64",
            "source_classes_survey_end_in_last64",
            "source_classes_survey_declared_byte_area_fits",
            "source_classes_survey_area_exact_0extra",
            "source_classes_survey_area_exact_1extra",
            "source_classes_survey_area_exact_2extra",
            "source_classes_survey_raw_bits20_in_range",
            "source_classes_survey_raw_bits24_in_range",
            "source_classes_survey_preamble_at20",
            "source_classes_survey_preamble_at24",
            "source_classes_survey_preamble_at28",
        )
        if type(hint.get("source_classes_header_survey_checked")) is not bool:
            raise ValueError("invalid_ac1024_class_survey_type")
        if any(type(hint.get(k)) is not bool for k in survey_fields):
            raise ValueError("invalid_ac1024_class_survey_field")
        if hint["source_classes_header_survey_checked"]:
            if not hint["source_classes_extracted"]:
                raise ValueError("unproven_ac1024_class_survey")
        elif any(hint[k] for k in survey_fields):
            raise ValueError("unproven_ac1024_class_survey_flags")

        positional_bool = (
            "source_classes_survey_end_in_last64_unique",
            "source_classes_survey_end_trailing_bytes_known",
            "source_classes_survey_declared_gap_known",
        )
        positional_int = (
            "source_classes_survey_end_trailing_bytes",
            "source_classes_survey_declared_gap_from20",
            "source_classes_survey_declared_gap_from24",
            "source_classes_survey_declared_gap_from28",
        )
        if any(type(hint.get(k)) is not bool for k in positional_bool):
            raise ValueError("invalid_ac1024_class_position_boolean")
        if any(type(hint.get(k)) is not int for k in positional_int):
            raise ValueError("invalid_ac1024_class_position_integer")
        if hint["source_classes_survey_end_in_last64_unique"] and not (
            hint["source_classes_header_survey_checked"]
            and hint["source_classes_survey_end_in_last64"]
        ):
            raise ValueError("unverified_ac1024_unique_close")
        if hint["source_classes_survey_end_trailing_bytes_known"] != (
            hint["source_classes_survey_end_in_last64_unique"]
        ):
            raise ValueError("inconsistent_ac1024_close_position")
        trailing = hint["source_classes_survey_end_trailing_bytes"]
        if not 0 <= trailing <= 48 or (
            not hint["source_classes_survey_end_trailing_bytes_known"]
            and trailing != 0
        ) or (
            hint["source_classes_survey_end_at_eof"]
            and hint["source_classes_survey_end_trailing_bytes_known"]
            and trailing != 0
        ):
            raise ValueError("invalid_ac1024_trailing_length")
        gap_keys = (
            "source_classes_survey_declared_gap_from20",
            "source_classes_survey_declared_gap_from24",
            "source_classes_survey_declared_gap_from28",
        )
        gaps = tuple(hint[k] for k in gap_keys)
        if hint["source_classes_survey_declared_gap_known"]:
            if not (hint["source_classes_survey_end_in_last64_unique"]
                    and hint["source_classes_survey_begin_at_zero"]
                    and hint["source_classes_survey_declared_byte_area_fits"]
                    and all(-8 * 1024 * 1024 <= x <= 8 * 1024 * 1024 for x in gaps)
                    and gaps[0] - gaps[1] == 4
                    and gaps[1] - gaps[2] == 4):
                raise ValueError("invalid_ac1024_class_size_gap")
        elif any(gaps):
            raise ValueError("unverified_ac1024_class_size_gap")
        survey = lambda k: hint["source_classes_survey_" + k]
        if (survey("begin_at_zero") and not survey("begin_in_first64")) or (
            survey("end_at_eof") and not survey("end_in_last64")
        ):
            raise ValueError("inconsistent_ac1024_sentinel_survey")
        if (survey("declared_byte_area_fits") and
                not survey("begin_at_zero")) or (
            any(survey(k) for k in (
                "area_exact_0extra", "area_exact_1extra", "area_exact_2extra"
            )) and not survey("declared_byte_area_fits")
        ):
            raise ValueError("inconsistent_ac1024_size_survey")
        if sum(int(survey(k)) for k in (
                "area_exact_0extra", "area_exact_1extra", "area_exact_2extra"
        )) > 1:
            raise ValueError("ambiguous_ac1024_size_survey")
        if not survey("begin_at_zero") and any(survey(k) for k in (
            "raw_bits20_in_range", "raw_bits24_in_range",
            "preamble_at20", "preamble_at24", "preamble_at28"
        )):
            raise ValueError("unverified_ac1024_numeric_shape")
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
        gap_fields = (
            "legacy_short_gap_1", "legacy_short_gap_2",
            "legacy_short_gap_3", "legacy_short_gap_4",
            "legacy_short_gap_5to16", "legacy_short_gap_17to64",
            "legacy_short_gap_over64",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in gap_fields):
            raise ValueError("invalid_ac1024_short_gap_histogram")
        if sum(hint[k] for k in gap_fields) != short:
            raise ValueError("incomplete_ac1024_short_gap_histogram")
        mc_fields = (
            "mc_frame_crc_pass", "mc_frame_crc_mismatch",
            "mc_frame_unreadable", "mc_frame_next_exact",
            "mc_byte_len_matches_short_gap", "mc_bytes_1",
            "mc_bytes_2", "mc_bytes_3", "mc_bytes_over3",
        )
        mc_complete = hint.get("mc_frame_probe_completed")
        if (type(mc_complete) is not bool or
                any(type(hint.get(k)) is not int or hint[k] < 0
                    for k in mc_fields)):
            raise ValueError("invalid_ac1024_mc_frame_contract")
        (mc_ok, mc_bad, mc_unreadable, mc_adjacent, mc_short_match,
         mc1, mc2, mc3, mc_more) = (hint[k] for k in mc_fields)
        if measured:
            if (not mc_complete or
                    mc_ok + mc_bad + mc_unreadable != handles or
                    mc1 + mc2 + mc3 + mc_more != mc_ok + mc_bad or
                    mc_adjacent > pairs or
                    mc_short_match > short or
                    mc_short_match > mc_adjacent):
                raise ValueError("incomplete_ac1024_mc_frame_evidence")
        elif mc_complete or any(hint[k] for k in mc_fields):
            raise ValueError("unmeasured_ac1024_mc_frame_claim")
        ot_fields = (
            "mc_ot_decoded", "mc_ot_unreadable",
            "mc_ot_raw_1", "mc_ot_raw_7", "mc_ot_raw_17",
            "mc_ot_raw_18", "mc_ot_raw_19", "mc_ot_raw_44",
            "mc_ot_raw_49", "mc_ot_raw_51", "mc_ot_raw_77",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in ot_fields):
            raise ValueError("invalid_ac1024_ot_type_contract")
        ot_decoded = hint["mc_ot_decoded"]
        ot_unreadable = hint["mc_ot_unreadable"]
        if measured:
            if (ot_decoded + ot_unreadable != mc_ok or
                    sum(hint[k] for k in ot_fields[2:]) > ot_decoded):
                raise ValueError("inconsistent_ac1024_ot_counts")
        elif any(hint[k] for k in ot_fields):
            raise ValueError("unmeasured_ac1024_ot_claim")
        core_fields = (
            "core_frame_verified", "core_frame_probe_failed",
            "core_raw_ot_or_span_mismatch",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in core_fields):
            raise ValueError("invalid_ac1024_core_probe_contract")
        core_ok, core_failed, core_mismatch = (hint[k] for k in core_fields)
        if measured:
            if (core_ok + core_failed != mc_ok or
                    core_mismatch > core_ok):
                raise ValueError("inconsistent_ac1024_core_probe_counts")
            if core_failed or core_mismatch:
                raise ValueError("firstparty_ac1024_core_disagrees_with_diagnostic")
        elif any(hint[k] for k in core_fields):
            raise ValueError("unmeasured_ac1024_core_probe_claim")
        bounds_fields = (
            "core_payload_bounds_verified", "core_payload_bounds_rejected",
            "core_payload_bounds_mismatch",
        )
        special_fields = (
            "mc_unreadable_invalid_ms", "mc_unreadable_short_or_boundary",
            "mc_unreadable_other", "mc_unreadable_first_half",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in bounds_fields + special_fields):
            raise ValueError("invalid_ac1024_bounds_or_special_contract")
        bounds_ok, bounds_failed, bounds_mismatch = (
            hint[k] for k in bounds_fields)
        invalid_ms, truncated, other, first_half = (
            hint[k] for k in special_fields)
        if measured:
            if (bounds_ok + bounds_failed != core_ok or
                    bounds_mismatch > bounds_ok or
                    bounds_failed or bounds_mismatch):
                raise ValueError("firstparty_ac1024_payload_bounds_mismatch")
            if (invalid_ms + truncated + other != mc_unreadable or
                    first_half > mc_unreadable):
                raise ValueError("incomplete_ac1024_special_record_triage")
        elif any(hint[k] for k in bounds_fields + special_fields):
            raise ValueError("unmeasured_ac1024_bounds_or_special_claim")
        stream_fields = (
            "core_stream_bounds_verified", "core_stream_bounds_rejected",
            "core_stream_bounds_mismatch", "core_empty_handle_streams",
        )
        sample_fields = (
            "sampled_first_h_attempts", "sampled_first_h_self_matches",
            "sampled_first_h_other", "sampled_first_h_unreadable",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in stream_fields + sample_fields):
            raise ValueError("invalid_ac1024_stream_or_h_sample_counts")
        stream_ok, stream_rejected, stream_mismatch, empty_streams = (
            hint[k] for k in stream_fields)
        attempts, self_matches, other_h, unreadable_h = (
            hint[k] for k in sample_fields)
        if measured:
            if (stream_ok + stream_rejected != core_ok or
                    stream_mismatch > stream_ok or
                    empty_streams > stream_ok or
                    stream_mismatch):
                raise ValueError("invalid_ac1024_handle_stream_bounds")
            if (not 1 <= attempts <= min(handles, 2048) or
                    self_matches + other_h + unreadable_h != attempts or
                    self_matches > stream_ok):
                raise ValueError("invalid_ac1024_first_handle_sample")
        elif any(hint[k] for k in stream_fields + sample_fields):
            raise ValueError("unmeasured_ac1024_handle_stream_claim")
        hcode_fields = (
            "sampled_first_h_code0_mismatch",
            "sampled_first_h_absolute_other_code",
            "sampled_first_h_relative_code",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in hcode_fields):
            raise ValueError("invalid_ac1024_first_h_code_counts")
        if sum(hint[k] for k in hcode_fields) != other_h:
            raise ValueError("incomplete_ac1024_first_h_code_breakdown")
        comparison_fields = (
            "sampled_first_h_reverse_bytes_matches",
            "sampled_first_h_plus_one_matches",
            "sampled_first_h_minus_one_matches",
            "sampled_first_h_one_byte_values",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in comparison_fields):
            raise ValueError("invalid_ac1024_handle_hypothesis_counters")
        if any(hint[k] > hint["sampled_first_h_code0_mismatch"]
               for k in comparison_fields):
            raise ValueError("unbounded_ac1024_handle_hypothesis_counters")

        membership_fields = (
            "sampled_first_h_in_handle_index",
            "sampled_first_h_not_in_handle_index",
            "sampled_first_h_same_index_offset",
            "sampled_first_h_count2", "sampled_first_h_count3",
            "sampled_first_h_count4to8",
            "sampled_scan_best_shift_bits",
            "sampled_scan_best_self_matches",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in membership_fields):
            raise ValueError("invalid_ac1024_handle_membership_contract")
        (in_map, out_map, same_offset, count2, count3, count4to8,
         best_shift, best_matches) = (hint[k] for k in membership_fields)
        if measured:
            decoded_h = attempts - unreadable_h
            if (in_map + out_map != decoded_h or
                    same_offset > in_map or
                    count2 + count3 + count4to8 > decoded_h or
                    best_shift > 32 or best_matches > attempts or
                    best_matches < self_matches or
                    same_offset < self_matches):
                raise ValueError("inconsistent_ac1024_first_h_map_membership")
        elif any(hint[k] for k in membership_fields):
            raise ValueError("unmeasured_ac1024_first_h_map_claim")
        self_h_evidence_fields = (
            "sampled_h_internally_distinct",
            "sampled_h_internal_duplicates",
            "sampled_h_successive_pairs",
            "sampled_h_successive_increases",
            "sampled_h_successive_decreases",
            "sampled_h_successive_equals",
            "sampled_h_less_than_index",
            "sampled_h_greater_than_index",
            "sampled_h_index_requires_over3_bytes",
            "sampled_h_low8_equal_index",
            "sampled_h_low16_equal_index",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in self_h_evidence_fields):
            raise ValueError("invalid_ac1024_first_h_internal_census")
        (distinct_h, duplicate_h, pairs_h, increases_h, decreases_h,
         equal_h, less_h, greater_h, wide_index_h, same_low8, same_low16) = (
            hint[k] for k in self_h_evidence_fields)
        if measured:
            decoded_h = attempts - unreadable_h
            if (distinct_h + duplicate_h != decoded_h or
                    pairs_h != max(decoded_h - 1, 0) or
                    increases_h + decreases_h + equal_h != pairs_h or
                    less_h + greater_h + self_matches != decoded_h or
                    any(v > decoded_h for v in (
                        wide_index_h, same_low8, same_low16)) or
                    same_low16 > same_low8 or
                    same_low16 < self_matches or
                    duplicate_h < equal_h):
                raise ValueError("inconsistent_ac1024_first_h_internal_census")
        elif any(hint[k] for k in self_h_evidence_fields):
            raise ValueError("unmeasured_ac1024_first_h_internal_census")
        alt_valid = hint.get("masked6_candidate_index_valid")
        alt_fields = (
            "masked6_candidate_handle_count", "masked6_sample_attempts",
            "masked6_sample_self_matches", "masked6_sample_other",
            "masked6_sample_unreadable",
        )
        if (type(alt_valid) is not bool or
                any(type(hint.get(k)) is not int or hint[k] < 0
                    for k in alt_fields)):
            raise ValueError("invalid_ac1024_alternate_map_contract")
        alt_count, alt_attempts, alt_self, alt_other, alt_unreadable = (
            hint[k] for k in alt_fields)
        if alt_valid:
            if (not measured or alt_count != handles or
                    alt_attempts < 1 or alt_attempts > 2048 or
                    alt_attempts > handles or
                    alt_self + alt_other + alt_unreadable != alt_attempts):
                raise ValueError("unproven_ac1024_alternate_map_sample")
        elif any(hint[k] for k in alt_fields):
            raise ValueError("unverified_ac1024_alternate_map_claim")
        census_complete = hint.get("full_embedded_h_census_completed")
        census_fields = (
            "full_embedded_h_attempts", "full_embedded_h_parsed_code0",
            "full_embedded_h_other_codes", "full_embedded_h_unreadable",
            "full_embedded_h_self_matches",
            "full_embedded_h_less_than_index",
            "full_embedded_h_greater_than_index",
            "full_embedded_h_neighbor_pairs",
            "full_embedded_h_increases", "full_embedded_h_decreases",
            "full_embedded_h_equals",
        )
        if (type(census_complete) is not bool or
                any(type(hint.get(k)) is not int or hint[k] < 0
                    for k in census_fields)):
            raise ValueError("invalid_ac1024_full_self_h_census")
        (full_attempts, full_parsed, full_other, full_bad,
         full_self, full_less, full_greater, full_pairs,
         full_up, full_down, full_equal) = (
            hint[k] for k in census_fields)
        if census_complete:
            if (not measured or full_attempts != handles or
                    full_parsed + full_other + full_bad != full_attempts or
                    full_self + full_less + full_greater != full_parsed or
                    full_pairs > max(full_parsed - 1, 0) or
                    full_up + full_down + full_equal != full_pairs or
                    full_self < self_matches):
                raise ValueError("incomplete_ac1024_full_self_h_census")
        elif any(hint[k] for k in census_fields):
            raise ValueError("unmeasured_ac1024_full_self_h_claim")
        recovered_attempted = hint.get("core_embedded_h_recovery_attempted")
        recovered_ok = hint.get("core_embedded_h_recovery_succeeded")
        recovered_count = hint.get("core_embedded_h_recovery_records")
        recovered_old_matches = hint.get(
            "core_embedded_h_recovery_original_index_matches")
        if (type(recovered_attempted) is not bool or
                type(recovered_ok) is not bool or
                type(recovered_count) is not int or
                type(recovered_old_matches) is not int or
                recovered_count < 0 or recovered_old_matches < 0):
            raise ValueError("invalid_ac1024_recovered_index_contract")
        if measured:
            if not recovered_attempted:
                raise ValueError("missing_ac1024_recovered_index_attempt")
            if recovered_ok:
                if (not census_complete or recovered_count != handles or
                        recovered_old_matches != full_self or
                        full_parsed != handles or full_other or full_bad or
                        full_up != handles - 1 or full_down or full_equal):
                    raise ValueError("unproven_ac1024_recovered_index")
            elif recovered_count or recovered_old_matches:
                raise ValueError("false_ac1024_recovered_index_count")
            elif (census_complete and full_parsed == handles and
                  full_up == handles - 1 and not full_down and
                  not full_equal and not full_other and not full_bad):
                raise ValueError("ac1024_reconciliation_failed_despite_valid_census")
        elif recovered_attempted or recovered_ok or recovered_count or recovered_old_matches:
            raise ValueError("unmeasured_ac1024_recovered_index_claim")
        zero_eed_attempts = hint.get("sampled_zero_eed_attempts")
        zero_eed_verified = hint.get("sampled_zero_eed_verified")
        zero_eed_refused = hint.get("sampled_zero_eed_refused")
        if (any(type(v) is not int or v < 0 for v in (
                zero_eed_attempts, zero_eed_verified, zero_eed_refused)) or
                zero_eed_attempts > 2048 or
                zero_eed_attempts != zero_eed_verified + zero_eed_refused):
            raise ValueError("invalid_ac1024_zero_eed_sample_contract")
        if recovered_ok:
            if zero_eed_attempts != min(handles, 2048):
                raise ValueError("incomplete_ac1024_zero_eed_sample")
        elif zero_eed_attempts or zero_eed_verified or zero_eed_refused:
            raise ValueError("unrecovered_ac1024_zero_eed_claim")
        no_proxy_verified = hint.get("sampled_no_proxy_verified")
        no_proxy_refused = hint.get("sampled_no_proxy_refused")
        if (type(no_proxy_verified) is not int or
                type(no_proxy_refused) is not int or
                no_proxy_verified < 0 or no_proxy_refused < 0 or
                no_proxy_verified + no_proxy_refused != zero_eed_attempts or
                no_proxy_verified > zero_eed_verified):
            raise ValueError("invalid_ac1024_no_proxy_sample_contract")
        entity_attempts = hint.get("sampled_entity_meta_attempts")
        entity_verified = hint.get("sampled_entity_meta_verified")
        entity_refused = hint.get("sampled_entity_meta_refused")
        if (any(type(n) is not int or n < 0 for n in (
                entity_attempts, entity_verified, entity_refused)) or
                entity_attempts > zero_eed_attempts or
                entity_verified + entity_refused != entity_attempts or
                entity_verified > no_proxy_verified):
            raise ValueError("invalid_ac1024_entity_meta_sample_contract")
        if not recovered_ok and (
                entity_attempts or entity_verified or entity_refused):
            raise ValueError("unrecovered_ac1024_entity_meta_claim")
        # The real native probe attempts owner/layer resolution once per
        # eligible fixed-type entity. A verified link is a strict subset
        # of CRC-backed common-entity metadata, never geometry or paint.
        link_attempts = hint.get("sampled_owner_layer_attempts")
        link_verified = hint.get("sampled_owner_layer_verified")
        link_refused = hint.get("sampled_owner_layer_refused")
        if (any(type(n) is not int or n < 0 for n in (
                link_attempts, link_verified, link_refused)) or
                link_attempts != entity_attempts or
                link_verified + link_refused != link_attempts or
                link_verified > entity_verified):
            raise ValueError("invalid_ac1024_owner_layer_sample_contract")
        if not recovered_ok and (
                link_attempts or link_verified or link_refused):
            raise ValueError("unrecovered_ac1024_owner_layer_claim")
        # Bounded samples of *original* LFS DWG raw OT19 LINE records.
        # A valid common prefix/XYZ field parse is NOT rendering, owner
        # linkage, independent geometric parity, or normal-open acceptance.
        # Independent first-party C11 per-LAYER source triplet proof.
        # A verified OT80/505/57 numeric sequence still cannot establish
        # CAD roles or make a partly consumed H-tail render-ready.
        triplet_keys = (
            "sampled_layer_eed_terminal_triplet_attempts",
            "sampled_layer_eed_terminal_triplet_verified",
            "sampled_layer_eed_terminal_triplet_nonzero_fragment",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0 for k in triplet_keys):
            raise ValueError("invalid_ac1024_raw_type_triplet_contract")
        triplet_attempted, triplet_verified, triplet_nonzero = (
            hint[k] for k in triplet_keys
        )
        candidate_verified = hint.get(
            "sampled_layer_eed_terminal_candidate_verified"
        )
        fragment_nonzero = hint.get(
            "sampled_layer_eed_terminal_fragment_nonzero"
        )
        if (type(candidate_verified) is not int or
                type(fragment_nonzero) is not int or
                triplet_attempted != candidate_verified or
                triplet_verified > triplet_attempted or
                triplet_nonzero > triplet_verified or
                triplet_nonzero > fragment_nonzero):
            raise ValueError("inconsistent_ac1024_type_triplet_provenance")

        alignment_keys = (
            "sampled_layer_eed_terminal_alignment_attempts",
            "sampled_layer_eed_terminal_alignment_verified",
            "sampled_layer_eed_terminal_alignment_nonzero",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0 for k in alignment_keys):
            raise ValueError("invalid_ac1024_alignment_witness_counts")
        alignment_attempts, alignment_verified, alignment_nonzero = (
            hint[k] for k in alignment_keys
        )
        if (alignment_attempts != triplet_verified or
                alignment_verified > alignment_attempts or
                alignment_nonzero > alignment_verified or
                alignment_nonzero > triplet_nonzero):
            raise ValueError("alignment_witness_without_native_triplet")

        raw_join_keys = (
            "sampled_layer_eed_state_join_attempts",
            "sampled_layer_eed_state_join_verified",
            "sampled_layer_eed_state_join_frozen",
            "sampled_layer_eed_state_join_on",
            "sampled_layer_eed_state_join_locked",
            "sampled_layer_eed_state_join_unmapped",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in raw_join_keys):
            raise ValueError("invalid_ac1024_same_record_raw_state_join")
        (join_attempts, join_verified, join_frozen,
         join_on, join_locked, join_unmapped) = (
             hint[k] for k in raw_join_keys
         )
        if (join_attempts != alignment_verified or
                join_verified > join_attempts or
                max(join_frozen, join_on, join_locked, join_unmapped)
                > join_verified or
                join_verified > hint.get("sampled_layer_eed_flags_verified", -1) or
                join_frozen > hint.get("sampled_layer_eed_flags_frozen_bit", -1) or
                join_on > hint.get("sampled_layer_eed_flags_on_bit", -1) or
                join_locked > hint.get("sampled_layer_eed_flags_locked_bit", -1) or
                join_unmapped > hint.get("sampled_layer_eed_flags_unmapped_bits", -1)):
            raise ValueError("unproven_ac1024_same_record_raw_flags_join")

        # Raw unresolved suffix is diagnostic only. Validate bit-pattern
        # masks against observed short fragments, never against CAD roles.
        mask2 = hint.get("sampled_layer_eed_terminal_fragment_2bit_pattern_mask")
        mask4 = hint.get("sampled_layer_eed_terminal_fragment_4bit_pattern_mask")
        starts = hint.get("sampled_layer_eed_terminal_fragment_start_mod8_mask")
        ends = hint.get("sampled_layer_eed_terminal_fragment_end_mod8_mask")
        f2 = hint.get("sampled_layer_eed_terminal_fragment_bits_2")
        f4 = hint.get("sampled_layer_eed_terminal_fragment_bits_4")
        fv = hint.get("sampled_layer_eed_terminal_fragment_verified")
        fzero = hint.get("sampled_layer_eed_terminal_fragment_zero")
        fnonzero = hint.get("sampled_layer_eed_terminal_fragment_nonzero")
        if (any(type(v) is not int for v in
                (mask2, mask4, starts, ends, f2, f4, fv, fzero, fnonzero))
                or not 0 <= mask2 <= 15 or not 0 <= mask4 <= 65535
                or not 0 <= starts <= 255 or not 0 <= ends <= 255
                or min(f2, f4, fv, fzero, fnonzero) < 0
                or f2 + f4 > fv or fzero + fnonzero != fv
                or bool(mask2) != bool(f2) or bool(mask4) != bool(f4)
                or bool(starts) != bool(fv) or bool(ends) != bool(fv)
                or mask2.bit_count() > f2 or mask4.bit_count() > f4
                or (fzero == 0 and ((mask2 & 1) or (mask4 & 1)))
                or (f2 + f4 == fv and fnonzero > 0
                    and mask2 <= 1 and mask4 <= 1)):
            raise ValueError("invalid_original_layer_terminal_raw_pattern")

        # EED LINE -> independently authenticated raw OT51 state/CMC
        # source-local evidence. A LINE indexed color 256 is only a
        # BYLAYER candidate, NOT proof of actual rendered pixel color.
        line_layer_keys = (
            "sampled_line_eed_layer_state_attempts",
            "sampled_line_eed_layer_state_verified",
            "sampled_line_eed_layer_state_refused",
            "sampled_line_eed_layer_state_frozen",
            "sampled_line_eed_layer_state_on",
            "sampled_line_eed_layer_state_locked",
            "sampled_line_eed_layer_state_raw_color_256",
            "sampled_line_eed_layer_state_explicit_rgb",
            "sampled_line_eed_layer_state_refused_flags",
            "sampled_line_eed_layer_state_refused_h_triplet",
            "sampled_line_eed_layer_state_refused_alignment",
            "sampled_line_eed_layer_state_refused_other",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in line_layer_keys):
            raise ValueError("invalid_source_local_line_layer_color_counts")
        (ll_attempts, ll_verified, ll_refused, ll_frozen,
         ll_on, ll_locked, ll_256, ll_rgb, ll_bad_flags,
         ll_bad_triplet, ll_bad_alignment, ll_bad_other) = (
             hint[k] for k in line_layer_keys
        )
        line_eed_verified = hint.get("sampled_line_eed_join_verified")
        if (type(line_eed_verified) is not int or line_eed_verified < 0 or
                ll_attempts != min(64, line_eed_verified) or
                ll_verified + ll_refused != ll_attempts or
                ll_bad_flags + ll_bad_triplet +
                ll_bad_alignment + ll_bad_other != ll_refused or
                max(ll_frozen, ll_on, ll_locked, ll_256, ll_rgb)
                > ll_verified):
            raise ValueError("unproven_source_local_line_layer_color_bridge")

        line_keys = (
            "sampled_line_attempts",
            "sampled_line_common_verified",
            "sampled_line_common_refused",
            "sampled_line_xyz_verified",
            "sampled_line_xyz_refused",
            "sampled_line_xyz_owner_layer_joined",
            "sampled_line_xyz_distinct_endpoints",
            "sampled_line_xyz_bounded_common",
            "sampled_line_meta_refused",
            "sampled_line_meta_self_refused",
            "sampled_line_meta_eed_refused",
            "sampled_line_meta_proxy_refused",
            "sampled_line_meta_entity_fields_refused",
            "sampled_line_eed_chain_attempts",
            "sampled_line_eed_chain_verified",
            "sampled_line_eed_chain_refused",
            "sampled_line_eed_chain_has_applications",
            "sampled_line_eed_xyz_attempts",
            "sampled_line_eed_xyz_verified",
            "sampled_line_eed_xyz_refused",
            "sampled_line_eed_xyz_distinct_endpoints",
            "sampled_line_eed_join_attempts",
            "sampled_line_eed_join_verified",
            "sampled_line_eed_join_refused",
            "sampled_line_eed_join_explicit_owner",
            "sampled_line_eed_join_implicit_layer",
            "sampled_line_eed_join_xdictionary",
            "sampled_line_eed_appid_attempts",
            "sampled_line_eed_appid_verified",
            "sampled_line_eed_appid_refused",
            "sampled_line_eed_appid_references_verified",
            "sampled_line_eed_appid_distinct_references_verified",
            "sampled_layer_raw_flags_attempts",
            "sampled_layer_raw_flags_verified",
            "sampled_layer_raw_flags_refused",
            "sampled_layer_flags_refused_index",
            "sampled_layer_flags_refused_eed",
            "sampled_layer_flags_refused_record",
            "sampled_layer_flags_refused_reactors",
            "sampled_layer_flags_refused_xdic_bit",
            "sampled_layer_flags_refused_numeric",
            "sampled_layer_flags_refused_other",
            "sampled_layer_raw_flags_frozen_bit",
            "sampled_layer_raw_flags_on_bit",
            "sampled_layer_raw_flags_locked_bit",
            "sampled_layer_raw_flags_unmapped_bits",
            "sampled_layer_raw_flags_xdictionary_present",
            "sampled_layer_eed_flags_attempts",
            "sampled_layer_eed_flags_verified",
            "sampled_layer_eed_flags_refused",
            "sampled_layer_eed_flags_frozen_bit",
            "sampled_layer_eed_flags_on_bit",
            "sampled_layer_eed_flags_locked_bit",
            "sampled_layer_eed_flags_unmapped_bits",
            "sampled_layer_eed_flags_xdictionary_present",
            "sampled_layer_eed_appid_attempts",
            "sampled_layer_eed_appid_verified",
            "sampled_layer_eed_appid_refused",
            "sampled_layer_eed_appid_refs_verified",
            "sampled_layer_eed_xdic_graph_attempts",
            "sampled_layer_eed_xdic_graph_verified",
            "sampled_layer_eed_xdic_graph_refused",
            "sampled_layer_eed_xdic_refused_identity",
            "sampled_layer_eed_xdic_refused_eed",
            "sampled_layer_eed_xdic_refused_flags",
            "sampled_layer_eed_xdic_refused_appid",
            "sampled_layer_eed_xdic_refused_tail",
            "sampled_layer_eed_xdic_refused_control_h",
            "sampled_layer_eed_xdic_refused_dictionary_h",
            "sampled_layer_eed_xdic_refused_target_index",
            "sampled_layer_eed_xdic_refused_target_class",
            "sampled_layer_eed_xdic_refused_target_crc",
            "sampled_layer_eed_xdic_refused_other",
            "sampled_layer_eed_first_h_attempts",
            "sampled_layer_eed_first_h_verified",
            "sampled_layer_eed_first_h_refused",
            "sampled_layer_eed_first_h_code_0",
            "sampled_layer_eed_first_h_code_1",
            "sampled_layer_eed_first_h_code_2",
            "sampled_layer_eed_first_h_code_3",
            "sampled_layer_eed_first_h_code_4",
            "sampled_layer_eed_first_h_code_5",
            "sampled_layer_eed_first_h_code_6",
            "sampled_layer_eed_first_h_code_7",
            "sampled_layer_eed_first_h_code_8",
            "sampled_layer_eed_first_h_code_9",
            "sampled_layer_eed_first_h_code_10",
            "sampled_layer_eed_first_h_code_11",
            "sampled_layer_eed_first_h_code_12",
            "sampled_layer_eed_first_h_code_13",
            "sampled_layer_eed_first_h_code_14",
            "sampled_layer_eed_first_h_code_15",
            "sampled_layer_eed_first_h_size_0",
            "sampled_layer_eed_first_h_size_1",
            "sampled_layer_eed_first_h_size_2",
            "sampled_layer_eed_first_h_size_other",
            "sampled_layer_eed_xdic_graph_has_tail",
            "sampled_layer_eed_tail_header_attempts",
            "sampled_layer_eed_tail_header_verified",
            "sampled_layer_eed_tail_header_refused",
            "sampled_layer_eed_tail_header_code_5",
            "sampled_layer_eed_tail_header_code_other",
            "sampled_layer_eed_tail_header_size_0",
            "sampled_layer_eed_tail_header_size_1",
            "sampled_layer_eed_tail_header_size_other",
            "sampled_layer_eed_tail_shape_attempts",
            "sampled_layer_eed_tail_shape_verified",
            "sampled_layer_eed_tail_shape_refused",
            "sampled_layer_eed_tail_shape_h5_zero_length",
            "sampled_layer_eed_tail_shape_other_raw_h",
            "sampled_layer_eed_tail_shape_token_count",
            "sampled_layer_eed_tail_shape_payload_octets",
            "sampled_layer_eed_tail_shape_h5_payload",
            "sampled_layer_eed_tail_shape_non_h5_zero",
            "sampled_layer_eed_tail_shape_non_h5_payload",
            "sampled_layer_eed_tail_shape_total_bits",
            "sampled_layer_eed_tail_shape_refused_precondition",
            "sampled_layer_eed_tail_shape_refused_stream",
            "sampled_layer_eed_tail_shape_refused_token_limit",
            "sampled_layer_eed_tail_shape_refused_short_header",
            "sampled_layer_eed_tail_shape_refused_header_read",
            "sampled_layer_eed_tail_shape_refused_length",
            "sampled_layer_eed_tail_shape_refused_length_overrun",
            "sampled_layer_eed_tail_shape_refused_payload",
            "sampled_layer_eed_tail_shape_refused_final",
            "sampled_layer_eed_tail_shape_refused_other",
            "sampled_layer_eed_terminal_fragment_attempts",
            "sampled_layer_eed_terminal_fragment_verified",
            "sampled_layer_eed_terminal_fragment_refused",
            "sampled_layer_eed_terminal_fragment_zero",
            "sampled_layer_eed_terminal_fragment_nonzero",
            "sampled_layer_eed_terminal_fragment_tokens",
            "sampled_layer_eed_terminal_fragment_h5_zero_tokens",
            "sampled_layer_eed_terminal_fragment_bits_1",
            "sampled_layer_eed_terminal_fragment_bits_2",
            "sampled_layer_eed_terminal_fragment_bits_3",
            "sampled_layer_eed_terminal_fragment_bits_4",
            "sampled_layer_eed_terminal_fragment_bits_5",
            "sampled_layer_eed_terminal_fragment_bits_6",
            "sampled_layer_eed_terminal_fragment_bits_7",
            "sampled_layer_eed_terminal_fragment_prefix_count",
            "sampled_layer_eed_terminal_fragment_prefix_code_5",
            "sampled_layer_eed_terminal_fragment_prefix_code_other",
            "sampled_layer_eed_terminal_fragment_prefix_size_zero",
            "sampled_layer_eed_terminal_fragment_prefix_size_nonzero",
            "sampled_layer_eed_terminal_fragment_prefix_four_tokens",
            "sampled_layer_eed_terminal_fragment_ordinal_1_code5",
            "sampled_layer_eed_terminal_fragment_ordinal_2_code5",
            "sampled_layer_eed_terminal_fragment_ordinal_3_code5",
            "sampled_layer_eed_terminal_fragment_ordinal_4_code5",
            "sampled_layer_eed_terminal_candidate_attempts",
            "sampled_layer_eed_terminal_candidate_verified",
            "sampled_layer_eed_terminal_candidate_refused",
            "sampled_layer_eed_terminal_candidate_prefix_tokens",
            "sampled_layer_eed_terminal_candidate_absolute",
            "sampled_layer_eed_terminal_candidate_matched",
            "sampled_layer_eed_terminal_candidate_unmatched",
            "sampled_layer_eed_terminal_candidate_other",
            "sampled_layer_eed_terminal_candidate_ot49",
            "sampled_layer_eed_terminal_candidate_ot51",
            "sampled_layer_eed_terminal_candidate_ot_other",
            "sampled_layer_eed_terminal_candidate_ot80_placeholder",
            "sampled_layer_eed_terminal_candidate_ot57_ltype",
            "sampled_layer_eed_terminal_candidate_dynamic_class_unresolved",
            "sampled_layer_eed_terminal_candidate_other_raw_ot",
            "sampled_layer_eed_terminal_candidate_ordinal_1_matched",
            "sampled_layer_eed_terminal_candidate_ordinal_1_raw_ot_consensus",
            "sampled_layer_eed_terminal_candidate_ordinal_2_matched",
            "sampled_layer_eed_terminal_candidate_ordinal_2_raw_ot_consensus",
            "sampled_layer_eed_terminal_candidate_ordinal_3_matched",
            "sampled_layer_eed_terminal_candidate_ordinal_3_raw_ot_consensus",
            "sampled_layer_eed_terminal_candidate_ordinal_4_matched",
            "sampled_layer_eed_terminal_candidate_ordinal_4_raw_ot_consensus",
            "sampled_line_common_after_meta_refused",
            "sampled_line_xyz_after_common_refused",
            "sampled_line_common_mode_unsupported",
            "sampled_line_common_reactors_unsupported",
            "sampled_line_common_xdic_unsupported",
            "sampled_line_common_fields_refused",
            "sampled_line_common_stage_color",
            "sampled_line_common_stage_rgb",
            "sampled_line_common_stage_transparency",
            "sampled_line_common_stage_ltype",
            "sampled_line_ltype_zero",
            "sampled_line_ltype_negative",
            "sampled_line_ltype_nonfinite",
            "sampled_line_ltype_parse_refused",
            "sampled_line_common_stage_appearance",
            "sampled_line_common_stage_geom_bound",
            "sampled_line_common_stage_other",
            "sampled_line_implicit_paper_xyz_verified",
            "sampled_line_implicit_model_xyz_verified",
            "sampled_line_graph_attempts",
            "sampled_line_graph_verified",
            "sampled_line_graph_refused",
            "sampled_line_implicit_layer_attempts",
            "sampled_line_implicit_layer_verified",
            "sampled_line_implicit_layer_refused",
            "sampled_line_xdic_present_attempts",
            "sampled_line_xdic_present_xyz_verified",
            "sampled_line_xdic_present_xyz_refused",
            "sampled_line_xdic_hgraph_attempts",
            "sampled_line_xdic_hgraph_verified",
            "sampled_line_xdic_hgraph_refused",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in line_keys):
            raise ValueError("invalid_ac1024_line_sample_type")
        (line_attempts, line_common_ok, line_common_bad,
         line_xyz_ok, line_xyz_bad, line_joined, line_distinct,
         line_bounded, line_meta_rejected,
         line_meta_self, line_meta_eed, line_meta_proxy, line_meta_fields,
         eed_chain_attempts, eed_chain_verified, eed_chain_refused,
         eed_chain_apps,
         eed_xyz_attempts, eed_xyz_verified, eed_xyz_refused,
         eed_xyz_distinct,
         eed_join_attempts, eed_join_ok, eed_join_bad,
         eed_join_explicit_owner, eed_join_implicit_layer, eed_join_xdic,
         eed_appid_attempts, eed_appid_ok, eed_appid_bad,
         eed_appid_refs, eed_appid_distinct,
         layer_flags_attempts, layer_flags_ok, layer_flags_bad,
         layer_refused_index, layer_refused_eed, layer_refused_record,
         layer_refused_reactors, layer_refused_xdic, layer_refused_numeric,
         layer_refused_other,
         layer_frozen, layer_on, layer_locked, layer_unmapped, layer_xdic,
         layer_eed_attempts, layer_eed_ok, layer_eed_bad,
         layer_eed_frozen, layer_eed_on, layer_eed_locked,
         layer_eed_unmapped, layer_eed_xdic,
         layer_appid_attempts, layer_appid_ok, layer_appid_bad,
         layer_appid_refs,
         layer_xdic_graph_attempts, layer_xdic_graph_ok,
         layer_xdic_graph_bad,
         layer_xdic_refused_identity, layer_xdic_refused_eed, layer_xdic_refused_flags, layer_xdic_refused_appid,
         layer_xdic_refused_tail, layer_xdic_refused_control_h, layer_xdic_refused_dictionary_h, layer_xdic_refused_target_index,
         layer_xdic_refused_target_class, layer_xdic_refused_target_crc, layer_xdic_refused_other,
         layer_first_h_attempts, layer_first_h_verified, layer_first_h_refused, layer_first_h_code_0, layer_first_h_code_1,
         layer_first_h_code_2, layer_first_h_code_3, layer_first_h_code_4, layer_first_h_code_5, layer_first_h_code_6,
         layer_first_h_code_7, layer_first_h_code_8, layer_first_h_code_9, layer_first_h_code_10, layer_first_h_code_11,
         layer_first_h_code_12, layer_first_h_code_13, layer_first_h_code_14, layer_first_h_code_15, layer_first_h_size_0,
         layer_first_h_size_1, layer_first_h_size_2, layer_first_h_size_other,
         layer_xdic_graph_has_tail,
         layer_tail_attempts, layer_tail_verified, layer_tail_refused,
         layer_tail_code_5, layer_tail_code_other,
         layer_tail_size_0, layer_tail_size_1, layer_tail_size_other,
         layer_shape_attempts, layer_shape_verified, layer_shape_refused,
         layer_shape_h5_zero, layer_shape_other, layer_shape_tokens, layer_shape_octets,
         layer_shape_h5_payload, layer_shape_non_h5_zero,
         layer_shape_non_h5_payload, layer_shape_total_bits,
         layer_shape_refused_precondition, layer_shape_refused_stream,
         layer_shape_refused_token_limit, layer_shape_refused_short_header,
         layer_shape_refused_header_read, layer_shape_refused_length,
         layer_shape_refused_length_overrun, layer_shape_refused_payload,
         layer_shape_refused_final, layer_shape_refused_other,
         fragment_attempts, fragment_verified, fragment_refused,
         fragment_zero, fragment_nonzero, fragment_tokens,
         fragment_h5_zero_tokens,
         fragment_bits_1, fragment_bits_2, fragment_bits_3,
         fragment_bits_4, fragment_bits_5, fragment_bits_6, fragment_bits_7,
         fragment_prefix_count, fragment_prefix_code_5,
         fragment_prefix_code_other, fragment_prefix_size_zero,
         fragment_prefix_size_nonzero, fragment_prefix_four_tokens,
         fragment_ordinal_1_code5, fragment_ordinal_2_code5,
         fragment_ordinal_3_code5, fragment_ordinal_4_code5,
         candidate_attempts, candidate_verified, candidate_refused, candidate_prefix_tokens, candidate_absolute,
         candidate_matched, candidate_unmatched, candidate_other, candidate_ot49, candidate_ot51, candidate_ot_other,
         candidate_ot80_placeholder, candidate_ot57_ltype,
         candidate_dynamic_class_unresolved, candidate_other_raw_ot,
         candidate_ordinal_1_matched,
         candidate_ordinal_1_raw_ot_consensus,
         candidate_ordinal_2_matched,
         candidate_ordinal_2_raw_ot_consensus,
         candidate_ordinal_3_matched,
         candidate_ordinal_3_raw_ot_consensus,
         candidate_ordinal_4_matched,
         candidate_ordinal_4_raw_ot_consensus,
         line_common_after_meta_rejected, line_xyz_after_common_rejected,
         line_mode_rejected, line_reactors_rejected, line_xdic_rejected,
         line_fields_rejected,
         stage_color, stage_rgb, stage_transparency, stage_ltype,
         ltype_zero, ltype_negative, ltype_nonfinite, ltype_parse,
         stage_appearance, stage_geom_bound, stage_other,
         implicit_paper_xyz,
         implicit_model_xyz, graph_attempts, graph_ok,
         graph_refused, implicit_layer_attempts,
         implicit_layer_ok, implicit_layer_bad,
         xdic_present_attempts, xdic_present_xyz_ok,
         xdic_present_xyz_bad, xdic_hgraph_attempts,
         xdic_hgraph_ok, xdic_hgraph_bad) = (hint[k] for k in line_keys)
        if (line_attempts > entity_attempts or
                line_attempts > 2048 or
                line_attempts != line_common_ok + line_common_bad or
                line_attempts != line_xyz_ok + line_xyz_bad or
                line_common_ok > entity_verified or
                line_xyz_ok > line_common_ok or
                line_bounded > line_xyz_ok or
                line_bounded > line_common_ok or
                line_joined > line_bounded or
                line_joined > link_verified or
                line_distinct > line_xyz_ok or
                implicit_paper_xyz + implicit_model_xyz > line_bounded - line_joined or
                graph_attempts != line_joined or
                graph_ok + graph_refused != graph_attempts or
                implicit_layer_attempts != implicit_paper_xyz + implicit_model_xyz or
                implicit_layer_ok + implicit_layer_bad != implicit_layer_attempts or
                xdic_present_attempts !=
                    xdic_present_xyz_ok + xdic_present_xyz_bad or
                xdic_present_attempts > line_attempts or
                xdic_present_xyz_ok > line_xyz_ok or
                xdic_present_xyz_bad > line_xyz_bad or
                xdic_hgraph_attempts != xdic_present_xyz_ok or
                xdic_hgraph_ok + xdic_hgraph_bad != xdic_hgraph_attempts or
                line_meta_rejected > entity_refused or
                line_meta_rejected != (
                    line_meta_self + line_meta_eed +
                    line_meta_proxy + line_meta_fields) or
                eed_chain_attempts != line_meta_eed or
                eed_chain_verified + eed_chain_refused != eed_chain_attempts or
                eed_chain_apps > eed_chain_verified or
                eed_xyz_attempts != eed_chain_apps or
                eed_xyz_verified + eed_xyz_refused != eed_xyz_attempts or
                eed_xyz_distinct > eed_xyz_verified or
                eed_join_attempts != eed_xyz_verified or
                eed_join_ok + eed_join_bad != eed_join_attempts or
                eed_join_explicit_owner + eed_join_implicit_layer != eed_join_ok or
                eed_join_xdic > eed_join_ok or
                eed_appid_attempts != eed_xyz_verified or
                eed_appid_ok + eed_appid_bad != eed_appid_attempts or
                eed_appid_refs < eed_appid_ok or
                eed_appid_refs > 64 * eed_appid_ok or
                eed_appid_distinct < eed_appid_ok or
                eed_appid_distinct > eed_appid_refs or
                layer_flags_attempts != layer_flags_ok + layer_flags_bad or
                layer_flags_bad != (
                    layer_refused_index + layer_refused_eed +
                    layer_refused_record + layer_refused_reactors +
                    layer_refused_xdic + layer_refused_numeric +
                    layer_refused_other) or
                max(layer_frozen, layer_on, layer_locked, layer_unmapped,
                    layer_xdic) > layer_flags_ok or
                layer_eed_attempts != layer_eed_ok + layer_eed_bad or
                layer_eed_attempts != layer_flags_attempts or
                layer_flags_ok + layer_eed_ok > layer_flags_attempts or
                max(layer_eed_frozen, layer_eed_on, layer_eed_locked,
                    layer_eed_unmapped, layer_eed_xdic) > layer_eed_ok or
                layer_appid_attempts != layer_eed_attempts or
                layer_appid_ok + layer_appid_bad != layer_appid_attempts or
                layer_appid_refs < layer_appid_ok or
                layer_appid_refs > 64 * layer_appid_ok or
                layer_xdic_graph_attempts != layer_appid_ok or
                layer_xdic_graph_ok + layer_xdic_graph_bad !=
                    layer_xdic_graph_attempts or
                layer_xdic_graph_bad != (
                    layer_xdic_refused_identity +
                    layer_xdic_refused_eed +
                    layer_xdic_refused_flags +
                    layer_xdic_refused_appid +
                    layer_xdic_refused_tail +
                    layer_xdic_refused_control_h +
                    layer_xdic_refused_dictionary_h +
                    layer_xdic_refused_target_index +
                    layer_xdic_refused_target_class +
                    layer_xdic_refused_target_crc + layer_xdic_refused_other) or
                layer_xdic_graph_has_tail > layer_xdic_graph_ok or
                layer_tail_attempts != layer_xdic_graph_has_tail or
                layer_tail_verified + layer_tail_refused != layer_tail_attempts or
                layer_tail_code_5 + layer_tail_code_other != layer_tail_verified or
                layer_tail_size_0 + layer_tail_size_1 + layer_tail_size_other != layer_tail_verified or
                layer_shape_attempts != layer_xdic_graph_has_tail or
                layer_shape_verified + layer_shape_refused != layer_shape_attempts or
                layer_shape_refused != (
                    layer_shape_refused_precondition + layer_shape_refused_stream +
                    layer_shape_refused_token_limit + layer_shape_refused_short_header +
                    layer_shape_refused_header_read + layer_shape_refused_length +
                    layer_shape_refused_length_overrun + layer_shape_refused_payload +
                    layer_shape_refused_final + layer_shape_refused_other) or
                fragment_attempts != layer_shape_refused_short_header or
                fragment_verified + fragment_refused != fragment_attempts or
                fragment_zero + fragment_nonzero != fragment_verified or
                (fragment_bits_1 + fragment_bits_2 + fragment_bits_3 +
                 fragment_bits_4 + fragment_bits_5 + fragment_bits_6 +
                 fragment_bits_7) != fragment_verified or
                fragment_tokens < fragment_verified or
                fragment_tokens > 64 * fragment_verified or
                fragment_h5_zero_tokens > fragment_tokens or
                fragment_prefix_count > fragment_tokens or
                fragment_prefix_count < fragment_verified or
                fragment_prefix_count > 4 * fragment_verified or
                fragment_prefix_count != (
                    fragment_prefix_code_5 + fragment_prefix_code_other) or
                fragment_prefix_count != (
                    fragment_prefix_size_zero + fragment_prefix_size_nonzero) or
                fragment_prefix_four_tokens > fragment_verified or
                (fragment_ordinal_1_code5 + fragment_ordinal_2_code5 +
                 fragment_ordinal_3_code5 + fragment_ordinal_4_code5) !=
                    fragment_prefix_code_5 or
                max(fragment_ordinal_1_code5, fragment_ordinal_2_code5,
                    fragment_ordinal_3_code5, fragment_ordinal_4_code5) >
                    fragment_verified or
                candidate_attempts != fragment_verified or
                candidate_verified + candidate_refused != candidate_attempts or
                candidate_prefix_tokens < candidate_verified or
                candidate_prefix_tokens > 4 * candidate_verified or
                candidate_absolute + candidate_other != candidate_prefix_tokens or
                candidate_matched + candidate_unmatched != candidate_absolute or
                candidate_ot49 + candidate_ot51 + candidate_ot_other !=
                    candidate_matched or
                candidate_ot80_placeholder + candidate_ot57_ltype +
                    candidate_dynamic_class_unresolved + candidate_other_raw_ot !=
                    candidate_matched or
                (candidate_ordinal_1_matched + candidate_ordinal_2_matched +
                 candidate_ordinal_3_matched + candidate_ordinal_4_matched) !=
                    candidate_matched or
                any(
                    (count > candidate_verified or
                     (count == 0 and consensus != 0) or
                     (count > 0 and (consensus == 0 or consensus > 65535)))
                    for count, consensus in (
                        (candidate_ordinal_1_matched, candidate_ordinal_1_raw_ot_consensus),
                        (candidate_ordinal_2_matched, candidate_ordinal_2_raw_ot_consensus),
                        (candidate_ordinal_3_matched, candidate_ordinal_3_raw_ot_consensus),
                        (candidate_ordinal_4_matched, candidate_ordinal_4_raw_ot_consensus))) or
                layer_shape_verified > layer_tail_verified or
                layer_shape_tokens < layer_shape_verified or
                layer_shape_tokens > 64 * layer_shape_verified or
                layer_shape_h5_zero + layer_shape_other != layer_shape_tokens or
                layer_shape_octets > 8 * layer_shape_tokens or
                layer_shape_other != (layer_shape_h5_payload +
                                      layer_shape_non_h5_zero +
                                      layer_shape_non_h5_payload) or
                layer_shape_total_bits !=
                    8 * (layer_shape_tokens + layer_shape_octets) or
                layer_first_h_attempts !=
                    layer_first_h_verified + layer_first_h_refused or
                layer_first_h_attempts != layer_eed_attempts or
                layer_first_h_verified != (
                    layer_first_h_code_0 +
                    layer_first_h_code_1 +
                    layer_first_h_code_2 +
                    layer_first_h_code_3 +
                    layer_first_h_code_4 +
                    layer_first_h_code_5 +
                    layer_first_h_code_6 +
                    layer_first_h_code_7 +
                    layer_first_h_code_8 +
                    layer_first_h_code_9 +
                    layer_first_h_code_10 +
                    layer_first_h_code_11 +
                    layer_first_h_code_12 +
                    layer_first_h_code_13 +
                    layer_first_h_code_14 +
                    layer_first_h_code_15) or
                layer_first_h_verified != (
                    layer_first_h_size_0 +
                    layer_first_h_size_1 +
                    layer_first_h_size_2 +
                    layer_first_h_size_other) or
                eed_xyz_verified + line_xyz_ok > line_attempts or
                line_common_bad != line_meta_rejected + line_common_after_meta_rejected or
                line_xyz_bad != line_common_bad + line_xyz_after_common_rejected or
                line_common_after_meta_rejected != (
                    line_mode_rejected + line_reactors_rejected +
                    line_xdic_rejected + line_fields_rejected) or
                line_common_after_meta_rejected != (
                    stage_color + stage_rgb + stage_transparency +
                    stage_ltype + stage_appearance + stage_geom_bound +
                    stage_other) or
                stage_ltype != (ltype_zero + ltype_negative +
                                ltype_nonfinite + ltype_parse)):
            raise ValueError("invalid_ac1024_line_sample_counts")
        if not recovered_ok and any(hint[k] for k in line_keys):
            raise ValueError("unrecovered_ac1024_line_sample_claim")
        ms_detail_fields = (
            "mc_invalid_ms_zero", "mc_invalid_ms_one",
            "mc_invalid_ms_2to7", "mc_invalid_ms_extended_zero",
            "mc_invalid_ms_extended_reserved",
            "mc_invalid_ms_unclassified",
        )
        if any(type(hint.get(k)) is not int or hint[k] < 0
               for k in ms_detail_fields):
            raise ValueError("invalid_ac1024_ms_reason_count")
        if sum(hint[k] for k in ms_detail_fields) != invalid_ms:
            raise ValueError("incomplete_ac1024_ms_reason_triage")
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
