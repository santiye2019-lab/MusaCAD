"""Fail-closed AC1024 compatibility observation; never a decoded entity index."""
from copy import deepcopy
import json
from pathlib import Path
import tempfile
import unittest

from real_dwg_native_stage_matrix import run_probe


def ac1024_probe_record():
    return {
        "signature": "AC1024",
        "result": "not_covered_by_this_native_probe",
        "native_stage": "ac1024_family_hypothesis_only",
        "evidence_count": 0,
        "geometry_verified": False,
        "rendering_verified": False,
        "source_modified": False,
        "ac1024_compatibility_hypothesis": {
            "patched_signature_in_memory_only": True,
            "validated_original_ac1024_decoder": False,
            "page_map_header": True,
            "physical_page_map": True,
            "objects_section_pages": True,
            "handles_section_pages": True,
            "candidate_handle_index": True,
            "candidate_handle_count": 145279,
            "source_page_map_header": False,
            "source_physical_page_map": False,
            "source_physical_page_count": 0,
            "source_patched_page_count_equal": False,
            "source_classes_section_verified": False,
            "source_classes_extracted": False,
            "source_classes_page_count": 0,
            "source_classes_logical_bytes": 0,
            "source_classes_numeric_no_extra": False,
            "source_classes_count_no_extra": 0,
            "source_classes_numeric_with_extra": False,
            "source_classes_count_with_extra": 0,
            "source_classes_r2010_bitcount_no_extra": False,
            "source_classes_r2010_bits_no_extra": 0,
            "source_classes_r2010_max_class_no_extra": 0,
            "source_classes_r2010_bitcount_with_extra": False,
            "source_classes_r2010_bits_with_extra": 0,
            "source_classes_r2010_max_class_with_extra": 0,
            "source_classes_r2010_tail8_with_extra": False,
            "source_classes_r2010_extent_exact_with_extra": False,
            "source_classes_r2010_string_span_bounded": False,
            "source_classes_r2010_string_bits": 0,
            "source_classes_r2010_string_wide_length": False,
            "source_classes_r2010_tu_verified": False,
            "source_classes_r2010_tu_fields": 0,
            "source_classes_r2010_tu_units": 0,
            "source_classes_r2010_tu_505_three_nonempty": False,
            "source_classes_r2010_tu_505_ascii_only": False,
            "source_classes_r2010_505_dxf_identifier_verified": False,
            "source_classes_r2010_505_dxf_identifier_length": 0,
            "source_classes_r2010_505_dxf_identifier": "",
            "source_classes_r2010_numeric_verified": False,
            "source_classes_r2010_ot505_present": False,
            "source_classes_r2010_ot505_entity": False,
            "source_classes_r2010_ot505_object": False,
            "source_classes_r2010_numeric_count": 0,
            "source_classes_header_survey_checked": False,
            "source_classes_survey_begin_at_zero": False,
            "source_classes_survey_end_at_eof": False,
            "source_classes_survey_begin_in_first64": False,
            "source_classes_survey_end_in_last64": False,
            "source_classes_survey_declared_byte_area_fits": False,
            "source_classes_survey_area_exact_0extra": False,
            "source_classes_survey_area_exact_1extra": False,
            "source_classes_survey_area_exact_2extra": False,
            "source_classes_survey_raw_bits20_in_range": False,
            "source_classes_survey_raw_bits24_in_range": False,
            "source_classes_survey_preamble_at20": False,
            "source_classes_survey_preamble_at24": False,
            "source_classes_survey_preamble_at28": False,
            "source_classes_survey_end_in_last64_unique": False,
            "source_classes_survey_end_trailing_bytes_known": False,
            "source_classes_survey_declared_gap_known": False,
            "source_classes_survey_end_trailing_bytes": 0,
            "source_classes_survey_declared_gap_from20": 0,
            "source_classes_survey_declared_gap_from24": 0,
            "source_classes_survey_declared_gap_from28": 0,
            "object_section_page_count": 558,
            "handle_section_page_count": 77,
            "legacy_frame_probe_completed": False,
            "legacy_frame_crc_ok": 0,
            "legacy_frame_crc_failed": 0,
            "legacy_raw_type_17_arc": 0,
            "legacy_raw_type_18_circle": 0,
            "legacy_raw_type_19_line": 0,
            "legacy_raw_type_77_lwpolyline": 0,
            "continuous_offset_candidate_index": False,
            "continuous_candidate_handles": 0,
            "continuous_frame_crc_ok": 0,
            "continuous_frame_crc_failed": 0,
            "page_absolute_candidate_index": False,
            "page_absolute_candidate_handles": 0,
            "page_absolute_frame_crc_ok": 0,
            "page_absolute_frame_crc_failed": 0,
            "legacy_offset_distribution_measured": False,
            "legacy_distinct_offsets": 0,
            "legacy_duplicate_offset_entries": 0,
            "legacy_zero_offset_entries": 0,
            "legacy_max_offset_multiplicity": 0,
            "legacy_distinct_frame_crc_pass": 0,
            "legacy_repeated_crc_pass_entries": 0,
            "legacy_frame_too_short": 0,
            "legacy_frame_invalid_ms": 0,
            "legacy_frame_exceeds_section": 0,
            "legacy_frame_crc_mismatch": 0,
            "legacy_shift_minus_two_crc_pass": 0,
            "legacy_shift_minus_one_crc_pass": 0,
            "legacy_shift_plus_one_crc_pass": 0,
            "legacy_shift_plus_two_crc_pass": 0,
            "legacy_shift_samples_tested": 0,
            "legacy_next_offset_pairs": 0,
            "legacy_next_frame_unavailable": 0,
            "legacy_next_frame_exact": 0,
            "legacy_next_frame_short": 0,
            "legacy_next_frame_overlaps": 0,
            "legacy_crc_ok_next_frame_exact": 0,
            "legacy_short_gap_1": 0,
            "legacy_short_gap_2": 0,
            "legacy_short_gap_3": 0,
            "legacy_short_gap_4": 0,
            "legacy_short_gap_5to16": 0,
            "legacy_short_gap_17to64": 0,
            "legacy_short_gap_over64": 0,
            "mc_frame_probe_completed": False,
            "mc_frame_crc_pass": 0,
            "mc_frame_crc_mismatch": 0,
            "mc_frame_unreadable": 0,
            "mc_frame_next_exact": 0,
            "mc_byte_len_matches_short_gap": 0,
            "mc_bytes_1": 0,
            "mc_bytes_2": 0,
            "mc_bytes_3": 0,
            "mc_bytes_over3": 0,
            "mc_ot_decoded": 0,
            "mc_ot_unreadable": 0,
            "mc_ot_raw_1": 0,
            "mc_ot_raw_7": 0,
            "mc_ot_raw_17": 0,
            "mc_ot_raw_18": 0,
            "mc_ot_raw_19": 0,
            "mc_ot_raw_44": 0,
            "mc_ot_raw_49": 0,
            "mc_ot_raw_51": 0,
            "mc_ot_raw_77": 0,
            "core_frame_verified": 0,
            "core_frame_probe_failed": 0,
            "core_raw_ot_or_span_mismatch": 0,
            "core_payload_bounds_verified": 0,
            "core_payload_bounds_rejected": 0,
            "core_payload_bounds_mismatch": 0,
            "mc_unreadable_invalid_ms": 0,
            "mc_unreadable_short_or_boundary": 0,
            "mc_unreadable_other": 0,
            "mc_unreadable_first_half": 0,
            "mc_invalid_ms_zero": 0,
            "mc_invalid_ms_one": 0,
            "mc_invalid_ms_2to7": 0,
            "mc_invalid_ms_extended_zero": 0,
            "mc_invalid_ms_extended_reserved": 0,
            "mc_invalid_ms_unclassified": 0,
            "core_stream_bounds_verified": 0,
            "core_stream_bounds_rejected": 0,
            "core_stream_bounds_mismatch": 0,
            "core_empty_handle_streams": 0,
            "sampled_first_h_attempts": 0,
            "sampled_first_h_self_matches": 0,
            "sampled_first_h_other": 0,
            "sampled_first_h_unreadable": 0,
            "sampled_first_h_code0_mismatch": 0,
            "sampled_first_h_absolute_other_code": 0,
            "sampled_first_h_relative_code": 0,
            "sampled_first_h_reverse_bytes_matches": 0,
            "sampled_first_h_plus_one_matches": 0,
            "sampled_first_h_minus_one_matches": 0,
            "sampled_first_h_one_byte_values": 0,
            "sampled_first_h_in_handle_index": 0,
            "sampled_first_h_not_in_handle_index": 0,
            "sampled_first_h_same_index_offset": 0,
            "sampled_first_h_count2": 0,
            "sampled_first_h_count3": 0,
            "sampled_first_h_count4to8": 0,
            "sampled_scan_best_shift_bits": 0,
            "sampled_scan_best_self_matches": 0,
            "sampled_h_internally_distinct": 0,
            "sampled_h_internal_duplicates": 0,
            "sampled_h_successive_pairs": 0,
            "sampled_h_successive_increases": 0,
            "sampled_h_successive_decreases": 0,
            "sampled_h_successive_equals": 0,
            "sampled_h_less_than_index": 0,
            "sampled_h_greater_than_index": 0,
            "sampled_h_index_requires_over3_bytes": 0,
            "sampled_h_low8_equal_index": 0,
            "sampled_h_low16_equal_index": 0,
            "masked6_candidate_index_valid": False,
            "masked6_candidate_handle_count": 0,
            "masked6_sample_attempts": 0,
            "masked6_sample_self_matches": 0,
            "masked6_sample_other": 0,
            "masked6_sample_unreadable": 0,
            "full_embedded_h_census_completed": False,
            "full_embedded_h_attempts": 0,
            "full_embedded_h_parsed_code0": 0,
            "full_embedded_h_other_codes": 0,
            "full_embedded_h_unreadable": 0,
            "full_embedded_h_self_matches": 0,
            "full_embedded_h_less_than_index": 0,
            "full_embedded_h_greater_than_index": 0,
            "full_embedded_h_neighbor_pairs": 0,
            "full_embedded_h_increases": 0,
            "full_embedded_h_decreases": 0,
            "full_embedded_h_equals": 0,
            "core_embedded_h_recovery_attempted": False,
            "core_embedded_h_recovery_succeeded": False,
            "core_embedded_h_recovery_records": 0,
            "core_embedded_h_recovery_original_index_matches": 0,
            "sampled_zero_eed_attempts": 0,
            "sampled_zero_eed_verified": 0,
            "sampled_zero_eed_refused": 0,
            "sampled_no_proxy_verified": 0,
            "sampled_no_proxy_refused": 0,
            "sampled_entity_meta_attempts": 0,
            "sampled_entity_meta_verified": 0,
            "sampled_entity_meta_refused": 0,
            "sampled_owner_layer_attempts": 0,
            "sampled_owner_layer_verified": 0,
            "sampled_owner_layer_refused": 0,
            "sampled_line_attempts": 0,
            "sampled_line_common_verified": 0,
            "sampled_line_common_refused": 0,
            "sampled_line_xyz_verified": 0,
            "sampled_line_xyz_refused": 0,
            "sampled_line_xyz_owner_layer_joined": 0,
            "sampled_line_xyz_distinct_endpoints": 0,
            "sampled_line_xyz_bounded_common": 0,
            "sampled_line_meta_refused": 0,
            "sampled_line_meta_self_refused": 0,
            "sampled_line_meta_eed_refused": 0,
            "sampled_line_meta_proxy_refused": 0,
            "sampled_line_meta_entity_fields_refused": 0,
            "sampled_line_eed_chain_attempts": 0,
            "sampled_line_eed_chain_verified": 0,
            "sampled_line_eed_chain_refused": 0,
            "sampled_line_eed_chain_has_applications": 0,
            "sampled_line_eed_xyz_attempts": 0,
            "sampled_line_eed_xyz_verified": 0,
            "sampled_line_eed_xyz_refused": 0,
            "sampled_line_eed_xyz_distinct_endpoints": 0,
            "sampled_line_eed_join_attempts": 0,
            "sampled_line_eed_join_verified": 0,
            "sampled_line_eed_join_refused": 0,
            "sampled_line_eed_join_explicit_owner": 0,
            "sampled_line_eed_join_implicit_layer": 0,
            "sampled_line_eed_join_xdictionary": 0,
            "sampled_line_eed_layer_state_attempts": 0,
            "sampled_line_eed_layer_state_verified": 0,
            "sampled_line_eed_layer_state_refused": 0,
            "sampled_line_eed_layer_state_frozen": 0,
            "sampled_line_eed_layer_state_on": 0,
            "sampled_line_eed_layer_state_locked": 0,
            "sampled_line_eed_layer_state_raw_color_256": 0,
            "sampled_line_eed_layer_state_raw_256_cmc_prefix_verified": 0,
            "sampled_line_eed_layer_state_raw_color_0": 0,
            "sampled_line_eed_layer_state_raw_color_1_255": 0,
            "sampled_line_eed_layer_state_raw_color_257": 0,
            "sampled_line_eed_layer_state_raw_color_other": 0,
            "sampled_line_eed_layer_state_explicit_rgb": 0,
            "sampled_line_eed_layer_state_refused_flags": 0,
            "sampled_line_eed_layer_state_refused_h_triplet": 0,
            "sampled_line_eed_layer_state_refused_alignment": 0,
            "sampled_line_eed_layer_state_refused_other": 0,
            "sampled_line_eed_appid_attempts": 0,
            "sampled_line_eed_appid_verified": 0,
            "sampled_line_eed_appid_refused": 0,
            "sampled_line_eed_appid_references_verified": 0,
            "sampled_line_eed_appid_distinct_references_verified": 0,
            "sampled_layer_raw_flags_attempts": 0,
            "sampled_layer_raw_flags_verified": 0,
            "sampled_layer_raw_flags_refused": 0,
            "sampled_layer_flags_refused_index": 0,
            "sampled_layer_flags_refused_eed": 0,
            "sampled_layer_flags_refused_record": 0,
            "sampled_layer_flags_refused_reactors": 0,
            "sampled_layer_flags_refused_xdic_bit": 0,
            "sampled_layer_flags_refused_numeric": 0,
            "sampled_layer_flags_refused_other": 0,
            "sampled_layer_raw_flags_frozen_bit": 0,
            "sampled_layer_raw_flags_on_bit": 0,
            "sampled_layer_raw_flags_locked_bit": 0,
            "sampled_layer_raw_flags_unmapped_bits": 0,
            "sampled_layer_raw_flags_xdictionary_present": 0,
            "sampled_layer_eed_flags_attempts": 0,
            "sampled_layer_eed_flags_verified": 0,
            "sampled_layer_eed_flags_refused": 0,
            "sampled_layer_eed_flags_frozen_bit": 0,
            "sampled_layer_eed_flags_on_bit": 0,
            "sampled_layer_eed_flags_locked_bit": 0,
            "sampled_layer_eed_flags_unmapped_bits": 0,
            "sampled_layer_eed_flags_xdictionary_present": 0,
            "sampled_layer_eed_appid_attempts": 0,
            "sampled_layer_eed_appid_verified": 0,
            "sampled_layer_eed_appid_refused": 0,
            "sampled_layer_eed_appid_refs_verified": 0,
            "sampled_layer_eed_xdic_graph_attempts": 0,
            "sampled_layer_eed_xdic_graph_verified": 0,
            "sampled_layer_eed_xdic_graph_refused": 0,
            "sampled_layer_eed_xdic_refused_identity": 0,
            "sampled_layer_eed_xdic_refused_eed": 0,
            "sampled_layer_eed_xdic_refused_flags": 0,
            "sampled_layer_eed_xdic_refused_appid": 0,
            "sampled_layer_eed_xdic_refused_tail": 0,
            "sampled_layer_eed_xdic_refused_control_h": 0,
            "sampled_layer_eed_xdic_refused_dictionary_h": 0,
            "sampled_layer_eed_xdic_refused_target_index": 0,
            "sampled_layer_eed_xdic_refused_target_class": 0,
            "sampled_layer_eed_xdic_refused_target_crc": 0,
            "sampled_layer_eed_xdic_refused_other": 0,
            "sampled_layer_eed_first_h_attempts": 0,
            "sampled_layer_eed_first_h_verified": 0,
            "sampled_layer_eed_first_h_refused": 0,
            "sampled_layer_eed_first_h_code_0": 0,
            "sampled_layer_eed_first_h_code_1": 0,
            "sampled_layer_eed_first_h_code_2": 0,
            "sampled_layer_eed_first_h_code_3": 0,
            "sampled_layer_eed_first_h_code_4": 0,
            "sampled_layer_eed_first_h_code_5": 0,
            "sampled_layer_eed_first_h_code_6": 0,
            "sampled_layer_eed_first_h_code_7": 0,
            "sampled_layer_eed_first_h_code_8": 0,
            "sampled_layer_eed_first_h_code_9": 0,
            "sampled_layer_eed_first_h_code_10": 0,
            "sampled_layer_eed_first_h_code_11": 0,
            "sampled_layer_eed_first_h_code_12": 0,
            "sampled_layer_eed_first_h_code_13": 0,
            "sampled_layer_eed_first_h_code_14": 0,
            "sampled_layer_eed_first_h_code_15": 0,
            "sampled_layer_eed_first_h_size_0": 0,
            "sampled_layer_eed_first_h_size_1": 0,
            "sampled_layer_eed_first_h_size_2": 0,
            "sampled_layer_eed_first_h_size_other": 0,
            "sampled_layer_eed_xdic_graph_has_tail": 0,
            "sampled_layer_eed_tail_header_attempts": 0,
            "sampled_layer_eed_tail_header_verified": 0,
            "sampled_layer_eed_tail_header_refused": 0,
            "sampled_layer_eed_tail_header_code_5": 0,
            "sampled_layer_eed_tail_header_code_other": 0,
            "sampled_layer_eed_tail_header_size_0": 0,
            "sampled_layer_eed_tail_header_size_1": 0,
            "sampled_layer_eed_tail_header_size_other": 0,
            "sampled_layer_eed_tail_shape_attempts": 0,
            "sampled_layer_eed_tail_shape_verified": 0,
            "sampled_layer_eed_tail_shape_refused": 0,
            "sampled_layer_eed_tail_shape_h5_zero_length": 0,
            "sampled_layer_eed_tail_shape_other_raw_h": 0,
            "sampled_layer_eed_tail_shape_token_count": 0,
            "sampled_layer_eed_tail_shape_payload_octets": 0,
            "sampled_layer_eed_tail_shape_h5_payload": 0,
            "sampled_layer_eed_tail_shape_non_h5_zero": 0,
            "sampled_layer_eed_tail_shape_non_h5_payload": 0,
            "sampled_layer_eed_tail_shape_total_bits": 0,
            "sampled_layer_eed_tail_shape_refused_precondition": 0,
            "sampled_layer_eed_tail_shape_refused_stream": 0,
            "sampled_layer_eed_tail_shape_refused_token_limit": 0,
            "sampled_layer_eed_tail_shape_refused_short_header": 0,
            "sampled_layer_eed_tail_shape_refused_header_read": 0,
            "sampled_layer_eed_tail_shape_refused_length": 0,
            "sampled_layer_eed_tail_shape_refused_length_overrun": 0,
            "sampled_layer_eed_tail_shape_refused_payload": 0,
            "sampled_layer_eed_tail_shape_refused_final": 0,
            "sampled_layer_eed_tail_shape_refused_other": 0,
            "sampled_layer_eed_terminal_fragment_attempts": 0,
            "sampled_layer_eed_terminal_fragment_verified": 0,
            "sampled_layer_eed_terminal_fragment_refused": 0,
            "sampled_layer_eed_terminal_fragment_zero": 0,
            "sampled_layer_eed_terminal_fragment_nonzero": 0,
            "sampled_layer_eed_terminal_fragment_tokens": 0,
            "sampled_layer_eed_terminal_fragment_h5_zero_tokens": 0,
            "sampled_layer_eed_terminal_fragment_bits_1": 0,
            "sampled_layer_eed_terminal_fragment_bits_2": 0,
            "sampled_layer_eed_terminal_fragment_bits_3": 0,
            "sampled_layer_eed_terminal_fragment_bits_4": 0,
            "sampled_layer_eed_terminal_fragment_bits_5": 0,
            "sampled_layer_eed_terminal_fragment_bits_6": 0,
            "sampled_layer_eed_terminal_fragment_bits_7": 0,
            "sampled_layer_eed_terminal_fragment_2bit_pattern_mask": 0,
            "sampled_layer_eed_terminal_fragment_4bit_pattern_mask": 0,
            "sampled_layer_eed_terminal_fragment_start_mod8_mask": 0,
            "sampled_layer_eed_terminal_fragment_end_mod8_mask": 0,
            "sampled_layer_eed_terminal_fragment_prefix_count": 0,
            "sampled_layer_eed_terminal_fragment_prefix_code_5": 0,
            "sampled_layer_eed_terminal_fragment_prefix_code_other": 0,
            "sampled_layer_eed_terminal_fragment_prefix_size_zero": 0,
            "sampled_layer_eed_terminal_fragment_prefix_size_nonzero": 0,
            "sampled_layer_eed_terminal_fragment_prefix_four_tokens": 0,
            "sampled_layer_eed_terminal_fragment_ordinal_1_code5": 0,
            "sampled_layer_eed_terminal_fragment_ordinal_2_code5": 0,
            "sampled_layer_eed_terminal_fragment_ordinal_3_code5": 0,
            "sampled_layer_eed_terminal_fragment_ordinal_4_code5": 0,
            "sampled_layer_eed_terminal_candidate_attempts": 0,
            "sampled_layer_eed_terminal_candidate_verified": 0,
            "sampled_layer_eed_terminal_triplet_attempts": 0,
            "sampled_layer_eed_terminal_triplet_verified": 0,
            "sampled_layer_eed_terminal_triplet_nonzero_fragment": 0,
            "sampled_layer_eed_terminal_alignment_attempts": 0,
            "sampled_layer_eed_terminal_alignment_verified": 0,
            "sampled_layer_eed_terminal_alignment_nonzero": 0,
            "sampled_layer_eed_state_join_attempts": 0,
            "sampled_layer_eed_state_join_verified": 0,
            "sampled_layer_eed_cmc_attempts": 0,
            "sampled_layer_eed_cmc_postflags_bs_attempts": 0,
            "sampled_layer_eed_cmc_postflags_bs_verified": 0,
            "sampled_layer_eed_cmc_postflags_bs_refused": 0,
            "sampled_layer_eed_cmc_postflags_bs_value_0": 0,
            "sampled_layer_eed_cmc_postflags_bs_value_1_255": 0,
            "sampled_layer_eed_cmc_postflags_bs_value_256": 0,
            "sampled_layer_eed_cmc_postflags_bs_value_257_1023": 0,
            "sampled_layer_eed_cmc_postflags_bs_value_1024_plus": 0,
            "sampled_layer_eed_cmc_verified": 0,
            "sampled_layer_eed_cmc_refused": 0,
            "sampled_layer_eed_cmc_refused_preflight": 0,
            "sampled_layer_eed_cmc_refused_index_nonzero": 0,
            "sampled_layer_eed_cmc_refused_bl": 0,
            "sampled_layer_eed_cmc_refused_rc": 0,
            "sampled_layer_eed_cmc_refused_rc_flags": 0,
            "sampled_layer_eed_cmc_refused_other": 0,
            "sampled_layer_eed_cmc_named": 0,
            "sampled_layer_eed_cmc_book_named": 0,
            "sampled_layer_eed_state_join_frozen": 0,
            "sampled_layer_eed_state_join_on": 0,
            "sampled_layer_eed_state_join_locked": 0,
            "sampled_layer_eed_state_join_unmapped": 0,
            "sampled_layer_eed_terminal_candidate_refused": 0,
            "sampled_layer_eed_terminal_candidate_prefix_tokens": 0,
            "sampled_layer_eed_terminal_candidate_absolute": 0,
            "sampled_layer_eed_terminal_candidate_matched": 0,
            "sampled_layer_eed_terminal_candidate_unmatched": 0,
            "sampled_layer_eed_terminal_candidate_other": 0,
            "sampled_layer_eed_terminal_candidate_ot49": 0,
            "sampled_layer_eed_terminal_candidate_ot51": 0,
            "sampled_layer_eed_terminal_candidate_ot_other": 0,
            "sampled_layer_eed_terminal_candidate_ot80_placeholder": 0,
            "sampled_layer_eed_terminal_candidate_ot57_ltype": 0,
            "sampled_layer_eed_terminal_candidate_dynamic_class_unresolved": 0,
            "sampled_layer_eed_terminal_candidate_other_raw_ot": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_1_matched": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_1_raw_ot_consensus": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_2_matched": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_2_raw_ot_consensus": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_3_matched": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_3_raw_ot_consensus": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_4_matched": 0,
            "sampled_layer_eed_terminal_candidate_ordinal_4_raw_ot_consensus": 0,
            "sampled_line_common_after_meta_refused": 0,
            "sampled_line_xyz_after_common_refused": 0,
            "sampled_line_common_mode_unsupported": 0,
            "sampled_line_common_reactors_unsupported": 0,
            "sampled_line_common_xdic_unsupported": 0,
            "sampled_line_common_fields_refused": 0,
            "sampled_line_common_stage_color": 0,
            "sampled_line_common_stage_rgb": 0,
            "sampled_line_common_stage_transparency": 0,
            "sampled_line_common_stage_ltype": 0,
            "sampled_line_ltype_zero": 0,
            "sampled_line_ltype_negative": 0,
            "sampled_line_ltype_nonfinite": 0,
            "sampled_line_ltype_parse_refused": 0,
            "sampled_line_common_stage_appearance": 0,
            "sampled_line_common_stage_geom_bound": 0,
            "sampled_line_common_stage_other": 0,
            "sampled_line_implicit_paper_xyz_verified": 0,
            "sampled_line_implicit_model_xyz_verified": 0,
            "sampled_line_graph_attempts": 0,
            "sampled_line_graph_verified": 0,
            "sampled_line_graph_refused": 0,
            "sampled_line_implicit_layer_attempts": 0,
            "sampled_line_implicit_layer_verified": 0,
            "sampled_line_implicit_layer_refused": 0,
            "sampled_line_xdic_present_attempts": 0,
            "sampled_line_xdic_present_xyz_verified": 0,
            "sampled_line_xdic_present_xyz_refused": 0,
            "sampled_line_xdic_hgraph_attempts": 0,
            "sampled_line_xdic_hgraph_verified": 0,
            "sampled_line_xdic_hgraph_refused": 0,
        }
    }


def validate_fake_result(record):
    with tempfile.TemporaryDirectory() as temp:
        reader = Path(temp) / "fake-probe"
        reader.write_text(
            "#!/usr/bin/env python3\n"
            "print(" + repr(json.dumps(record)) + ")\n",
            encoding="utf-8",
        )
        reader.chmod(0o755)
        source = Path(temp) / "fixture.dwg"
        source.write_bytes(b"AC1024")
        return run_probe(reader, source, 5.0)


class Ac1024CompatibilityContractTest(unittest.TestCase):
    def test_source_terminal_bit_patterns_need_matching_witness(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for field, value in (
            ("sampled_layer_eed_terminal_fragment_2bit_pattern_mask", 2),
            ("sampled_layer_eed_terminal_fragment_4bit_pattern_mask", 8),
            ("sampled_layer_eed_terminal_fragment_start_mod8_mask", 1),
            ("sampled_layer_eed_terminal_fragment_end_mod8_mask", 1),
        ):
            bad = deepcopy(original)
            bad["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_fake_result(bad)

    def test_source_local_line_layer_color_cannot_be_fabricated(self):
        baseline = ac1024_probe_record()
        self.assertEqual(validate_fake_result(baseline), baseline)
        for key in (
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
        ):
            fake = deepcopy(baseline)
            fake["ac1024_compatibility_hypothesis"][key] = 1
            with self.subTest(field=key), self.assertRaises(ValueError):
                validate_fake_result(fake)

    def test_original_postflags_bs_candidate_never_claims_cmc_color(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for key in (
            "sampled_layer_eed_cmc_postflags_bs_attempts",
            "sampled_layer_eed_cmc_postflags_bs_verified",
            "sampled_layer_eed_cmc_postflags_bs_value_0",
            "sampled_layer_eed_cmc_postflags_bs_value_1_255",
            "sampled_layer_eed_cmc_postflags_bs_value_256",
            "sampled_layer_eed_cmc_postflags_bs_value_257_1023",
            "sampled_layer_eed_cmc_postflags_bs_value_1024_plus",
        ):
            fake = deepcopy(original)
            fake["ac1024_compatibility_hypothesis"][key] = 1
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(fake)

    def test_layer_cmc_counts_require_independent_original_prefix(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for key in (
            "sampled_layer_eed_cmc_attempts",
            "sampled_layer_eed_cmc_verified",
            "sampled_layer_eed_cmc_refused",
            "sampled_layer_eed_cmc_named",
            "sampled_layer_eed_cmc_book_named",
            "sampled_layer_eed_cmc_refused_preflight",
            "sampled_layer_eed_cmc_refused_index_nonzero",
            "sampled_layer_eed_cmc_refused_bl",
            "sampled_layer_eed_cmc_refused_rc",
            "sampled_layer_eed_cmc_refused_rc_flags",
            "sampled_layer_eed_cmc_refused_other",
            "sampled_line_eed_layer_state_raw_256_cmc_prefix_verified",
        ):
            bad = deepcopy(original)
            bad["ac1024_compatibility_hypothesis"][key] = 1
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(bad)

    def test_original_color_index_bucket_partition_is_fail_closed(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)
        for key in ("sampled_line_eed_layer_state_raw_color_0",
                    "sampled_line_eed_layer_state_raw_color_1_255",
                    "sampled_line_eed_layer_state_raw_color_256",
                    "sampled_line_eed_layer_state_raw_color_257",
                    "sampled_line_eed_layer_state_raw_color_other"):
            bad = deepcopy(record)
            bad["ac1024_compatibility_hypothesis"][key] = 1
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(bad)

    def test_original_layer_type_triplet_cannot_be_fabricated(self):
        baseline = ac1024_probe_record()
        self.assertEqual(validate_fake_result(baseline), baseline)
        for key in (
            "sampled_layer_eed_terminal_triplet_attempts",
            "sampled_layer_eed_terminal_triplet_verified",
            "sampled_layer_eed_terminal_triplet_nonzero_fragment",
            "sampled_layer_eed_terminal_alignment_attempts",
            "sampled_layer_eed_terminal_alignment_verified",
            "sampled_layer_eed_terminal_alignment_nonzero",
            "sampled_layer_eed_state_join_attempts",
            "sampled_layer_eed_state_join_verified",
            "sampled_layer_eed_state_join_frozen",
            "sampled_layer_eed_state_join_on",
            "sampled_layer_eed_state_join_locked",
            "sampled_layer_eed_state_join_unmapped",
        ):
            fake = deepcopy(baseline)
            fake["ac1024_compatibility_hypothesis"][key] = 1
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(fake)

    def test_r2010_ot505_numeric_source_requirements(self):
        baseline = ac1024_probe_record()
        self.assertEqual(validate_fake_result(baseline), baseline)
        for key,value in (
            ("source_classes_r2010_numeric_verified", True),
            ("source_classes_r2010_numeric_count", 1),
            ("source_classes_r2010_ot505_present", True),
            ("source_classes_r2010_ot505_entity", True),
            ("source_classes_r2010_ot505_object", True),
        ):
            bad=deepcopy(baseline)
            bad["ac1024_compatibility_hypothesis"][key]=value
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(bad)
        valid=deepcopy(baseline)
        hint=valid["ac1024_compatibility_hypothesis"]
        hint["source_page_map_header"]=True
        hint["source_physical_page_map"]=True
        hint["source_physical_page_count"]=2
        hint["source_classes_section_verified"]=True
        hint["source_classes_extracted"]=True
        hint["source_classes_page_count"]=1
        hint["source_classes_logical_bytes"]=100
        hint["source_classes_r2010_bitcount_with_extra"]=True
        hint["source_classes_r2010_bits_with_extra"]=512
        hint["source_classes_r2010_max_class_with_extra"]=505
        hint["source_classes_r2010_numeric_verified"]=True
        hint["source_classes_r2010_numeric_count"]=1
        hint["source_classes_r2010_ot505_present"]=True
        hint["source_classes_r2010_ot505_object"]=True
        self.assertEqual(validate_fake_result(valid),valid)
        hint["source_classes_r2010_ot505_entity"]=True
        with self.assertRaises(ValueError):
            validate_fake_result(valid)

    def test_r2010_string_span_never_acts_as_named_class(self):
        baseline=ac1024_probe_record()
        self.assertEqual(validate_fake_result(baseline),baseline)
        for key,value in (
            ("source_classes_r2010_string_span_bounded", True),
            ("source_classes_r2010_string_bits", 8),
            ("source_classes_r2010_string_wide_length", True),
        ):
            invalid=deepcopy(baseline)
            invalid["ac1024_compatibility_hypothesis"][key]=value
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(invalid)
        valid=deepcopy(baseline)
        h=valid["ac1024_compatibility_hypothesis"]
        h["source_page_map_header"]=True
        h["source_physical_page_map"]=True
        h["source_physical_page_count"]=2
        h["source_classes_section_verified"]=True
        h["source_classes_extracted"]=True
        h["source_classes_page_count"]=1
        h["source_classes_logical_bytes"]=100
        h["source_classes_r2010_bitcount_with_extra"]=True
        h["source_classes_r2010_bits_with_extra"]=512
        h["source_classes_r2010_max_class_with_extra"]=505
        h["source_classes_r2010_extent_exact_with_extra"]=True
        h["source_classes_r2010_string_span_bounded"]=True
        h["source_classes_r2010_string_bits"]=8
        self.assertEqual(validate_fake_result(valid),valid)
        h["source_classes_r2010_string_bits"]=500
        with self.assertRaises(ValueError):
            validate_fake_result(valid)
        h["source_classes_r2010_string_bits"]=8
        h["source_classes_r2010_bitcount_with_extra"]=False
        h["source_classes_r2010_bits_with_extra"]=0
        h["source_classes_r2010_max_class_with_extra"]=0
        with self.assertRaises(ValueError):
            validate_fake_result(valid)

    def test_r2010_tu_three_fields_need_original_numeric_and_span(self):
        baseline=ac1024_probe_record()
        self.assertEqual(validate_fake_result(baseline),baseline)
        for field,value in (
            ("source_classes_r2010_tu_verified",True),
            ("source_classes_r2010_tu_fields",3),
            ("source_classes_r2010_tu_units",6),
            ("source_classes_r2010_tu_505_three_nonempty",True),
            ("source_classes_r2010_tu_505_ascii_only",True),
        ):
            invalid=deepcopy(baseline)
            invalid["ac1024_compatibility_hypothesis"][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):
                validate_fake_result(invalid)
        valid=deepcopy(baseline)
        h=valid["ac1024_compatibility_hypothesis"]
        h["source_page_map_header"]=True
        h["source_physical_page_map"]=True
        h["source_physical_page_count"]=2
        h["source_classes_section_verified"]=True
        h["source_classes_extracted"]=True
        h["source_classes_page_count"]=1
        h["source_classes_logical_bytes"]=100
        h["source_classes_r2010_bitcount_with_extra"]=True
        h["source_classes_r2010_bits_with_extra"]=512
        h["source_classes_r2010_max_class_with_extra"]=505
        h["source_classes_r2010_extent_exact_with_extra"]=True
        h["source_classes_r2010_numeric_verified"]=True
        h["source_classes_r2010_numeric_count"]=1
        h["source_classes_r2010_ot505_present"]=True
        h["source_classes_r2010_ot505_object"]=True
        h["source_classes_r2010_string_span_bounded"]=True
        h["source_classes_r2010_string_bits"]=128
        h["source_classes_r2010_tu_verified"]=True
        h["source_classes_r2010_tu_fields"]=3
        h["source_classes_r2010_tu_units"]=3
        h["source_classes_r2010_tu_505_three_nonempty"]=True
        h["source_classes_r2010_tu_505_ascii_only"]=True
        self.assertEqual(validate_fake_result(valid),valid)
        h["source_classes_r2010_tu_fields"]=2
        with self.assertRaises(ValueError):
            validate_fake_result(valid)
        h["source_classes_r2010_tu_fields"]=3
        h["source_classes_r2010_numeric_verified"]=False
        h["source_classes_r2010_numeric_count"]=0
        with self.assertRaises(ValueError):
            validate_fake_result(valid)

    def test_real_class505_dxf_name_is_strictly_proven_and_sanitized(self):
        baseline=ac1024_probe_record()
        self.assertEqual(validate_fake_result(baseline),baseline)
        for key,value in (
            ("source_classes_r2010_505_dxf_identifier_verified",True),
            ("source_classes_r2010_505_dxf_identifier_length",4),
            ("source_classes_r2010_505_dxf_identifier","TEST"),
        ):
            invalid=deepcopy(baseline)
            invalid["ac1024_compatibility_hypothesis"][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):
                validate_fake_result(invalid)
        valid=deepcopy(baseline)
        h=valid["ac1024_compatibility_hypothesis"]
        h["source_page_map_header"]=True
        h["source_physical_page_map"]=True
        h["source_physical_page_count"]=2
        h["source_classes_section_verified"]=True
        h["source_classes_extracted"]=True
        h["source_classes_page_count"]=1
        h["source_classes_logical_bytes"]=100
        h["source_classes_r2010_bitcount_with_extra"]=True
        h["source_classes_r2010_bits_with_extra"]=512
        h["source_classes_r2010_max_class_with_extra"]=505
        h["source_classes_r2010_extent_exact_with_extra"]=True
        h["source_classes_r2010_numeric_verified"]=True
        h["source_classes_r2010_numeric_count"]=1
        h["source_classes_r2010_ot505_present"]=True
        h["source_classes_r2010_ot505_object"]=True
        h["source_classes_r2010_string_span_bounded"]=True
        h["source_classes_r2010_string_bits"]=128
        h["source_classes_r2010_tu_verified"]=True
        h["source_classes_r2010_tu_fields"]=3
        h["source_classes_r2010_tu_units"]=3
        h["source_classes_r2010_tu_505_three_nonempty"]=True
        h["source_classes_r2010_tu_505_ascii_only"]=True
        h["source_classes_r2010_505_dxf_identifier_verified"]=True
        h["source_classes_r2010_505_dxf_identifier_length"]=4
        h["source_classes_r2010_505_dxf_identifier"]="TEST"
        self.assertEqual(validate_fake_result(valid),valid)
        for key,value in (
            ("source_classes_r2010_505_dxf_identifier_length",5),
            ("source_classes_r2010_505_dxf_identifier","TEST\n"),
            ("source_classes_r2010_505_dxf_identifier","A B"),
            ("source_classes_r2010_tu_verified",False),
            ("source_classes_r2010_tu_505_three_nonempty",False),
            ("source_classes_r2010_tu_505_ascii_only",False),
        ):
            invalid=deepcopy(valid)
            invalid["ac1024_compatibility_hypothesis"][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):
                validate_fake_result(invalid)

    def test_r2010_tail_evidence_requires_validated_header(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)
        for key in (
            "source_classes_r2010_tail8_with_extra",
            "source_classes_r2010_extent_exact_with_extra",
        ):
            bad = deepcopy(record)
            bad["ac1024_compatibility_hypothesis"][key] = True
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_fake_result(bad)

    def test_real_class_close_gap_contract_is_fail_closed(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)
        for field,value in (
            ("source_classes_survey_end_in_last64_unique", True),
            ("source_classes_survey_end_trailing_bytes_known", True),
            ("source_classes_survey_declared_gap_known", True),
            ("source_classes_survey_end_trailing_bytes", 8),
            ("source_classes_survey_declared_gap_from28", -4),
        ):
            bad = deepcopy(record)
            bad["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_fake_result(bad)
        good = deepcopy(record)
        hint = good["ac1024_compatibility_hypothesis"]
        hint["source_page_map_header"] = True
        hint["source_physical_page_map"] = True
        hint["source_physical_page_count"] = 2
        hint["source_classes_section_verified"] = True
        hint["source_classes_extracted"] = True
        hint["source_classes_page_count"] = 1
        hint["source_classes_logical_bytes"] = 4096
        hint["source_classes_header_survey_checked"] = True
        hint["source_classes_survey_begin_at_zero"] = True
        hint["source_classes_survey_begin_in_first64"] = True
        hint["source_classes_survey_end_in_last64"] = True
        hint["source_classes_survey_declared_byte_area_fits"] = True
        hint["source_classes_survey_end_in_last64_unique"] = True
        hint["source_classes_survey_end_trailing_bytes_known"] = True
        hint["source_classes_survey_end_trailing_bytes"] = 8
        hint["source_classes_survey_declared_gap_known"] = True
        hint["source_classes_survey_declared_gap_from20"] = 8
        hint["source_classes_survey_declared_gap_from24"] = 4
        hint["source_classes_survey_declared_gap_from28"] = 0
        self.assertEqual(validate_fake_result(good), good)
        hint["source_classes_survey_declared_gap_from24"] = 3
        with self.assertRaises(ValueError):
            validate_fake_result(good)

    def test_original_class_survey_is_strictly_structural(self):
        valid = ac1024_probe_record()
        self.assertEqual(validate_fake_result(valid), valid)
        for changed, value in (
            ("source_classes_header_survey_checked", 1),
            ("source_classes_survey_begin_at_zero", True),
            ("source_classes_survey_preamble_at28", True),
            ("source_classes_survey_area_exact_1extra", True),
        ):
            bad = deepcopy(valid)
            bad["ac1024_compatibility_hypothesis"][changed] = value
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                validate_fake_result(bad)
        valid = deepcopy(ac1024_probe_record())
        hint = valid["ac1024_compatibility_hypothesis"]
        hint["source_page_map_header"] = True
        hint["source_physical_page_map"] = True
        hint["source_physical_page_count"] = 2
        hint["source_classes_section_verified"] = True
        hint["source_classes_extracted"] = True
        hint["source_classes_page_count"] = 1
        hint["source_classes_logical_bytes"] = 4096
        hint["source_classes_header_survey_checked"] = True
        hint["source_classes_survey_begin_at_zero"] = True
        hint["source_classes_survey_begin_in_first64"] = True
        hint["source_classes_survey_end_at_eof"] = True
        hint["source_classes_survey_end_in_last64"] = True
        hint["source_classes_survey_declared_byte_area_fits"] = True
        hint["source_classes_survey_area_exact_1extra"] = True
        self.assertEqual(validate_fake_result(valid), valid)
        hint["source_classes_survey_area_exact_0extra"] = True
        with self.assertRaises(ValueError):
            validate_fake_result(valid)

    def test_r2010_bitcount_prefix_never_promotes_unverified_classes(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)
        for field, value in (
            ("source_classes_r2010_bitcount_no_extra", True),
            ("source_classes_r2010_bits_no_extra", 96),
            ("source_classes_r2010_max_class_no_extra", 505),
            ("source_classes_r2010_bitcount_with_extra", True),
            ("source_classes_r2010_bits_with_extra", 96),
            ("source_classes_r2010_max_class_with_extra", 505),
        ):
            wrong = deepcopy(record)
            wrong["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_fake_result(wrong)
        good = deepcopy(record)
        hint = good["ac1024_compatibility_hypothesis"]
        hint["source_page_map_header"] = True
        hint["source_physical_page_map"] = True
        hint["source_physical_page_count"] = 2
        hint["source_classes_section_verified"] = True
        hint["source_classes_extracted"] = True
        hint["source_classes_page_count"] = 1
        hint["source_classes_logical_bytes"] = 96
        hint["source_classes_r2010_bitcount_no_extra"] = True
        hint["source_classes_r2010_bits_no_extra"] = 96
        hint["source_classes_r2010_max_class_no_extra"] = 505
        self.assertEqual(validate_fake_result(good), good)
        hint["source_classes_r2010_bits_no_extra"] = 0
        with self.assertRaises(ValueError):
            validate_fake_result(good)

    def test_original_ac1024_classes_require_original_crc_provenance(self):
        valid = ac1024_probe_record()
        self.assertEqual(validate_fake_result(valid), valid)
        for field, value in (
            ("source_classes_section_verified", True),
            ("source_classes_extracted", True),
            ("source_classes_logical_bytes", 32),
            ("source_classes_page_count", 1),
            ("source_classes_numeric_no_extra", True),
            ("source_classes_numeric_with_extra", True),
            ("source_classes_count_no_extra", 2),
            ("source_classes_count_with_extra", 1),
        ):
            bad = deepcopy(valid)
            bad["ac1024_compatibility_hypothesis"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_fake_result(bad)
        good = deepcopy(valid)
        hint = good["ac1024_compatibility_hypothesis"]
        hint["source_page_map_header"] = True
        hint["source_physical_page_map"] = True
        hint["source_physical_page_count"] = 3
        hint["source_classes_section_verified"] = True
        hint["source_classes_extracted"] = True
        hint["source_classes_logical_bytes"] = 128
        hint["source_classes_page_count"] = 1
        hint["source_classes_numeric_no_extra"] = True
        hint["source_classes_count_no_extra"] = 1
        self.assertEqual(validate_fake_result(good), good)

    def test_original_ac1024_page_map_claims_are_fail_closed(self):
        valid = ac1024_probe_record()
        self.assertEqual(validate_fake_result(valid), valid)
        for key, value in (
            ("source_page_map_header", 1),
            ("source_physical_page_map", "true"),
            ("source_physical_page_count", True),
            ("source_physical_page_count", -1),
            ("source_physical_page_count", 131073),
        ):
            record = deepcopy(valid)
            record["ac1024_compatibility_hypothesis"][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                validate_fake_result(record)
        record = deepcopy(valid)
        record["ac1024_compatibility_hypothesis"]["source_physical_page_map"] = True
        with self.assertRaises(ValueError):
            validate_fake_result(record)
        record = deepcopy(valid)
        record["ac1024_compatibility_hypothesis"]["source_patched_page_count_equal"] = True
        with self.assertRaises(ValueError):
            validate_fake_result(record)
        record = deepcopy(valid)
        hint = record["ac1024_compatibility_hypothesis"]
        hint["source_page_map_header"] = True
        hint["source_physical_page_map"] = True
        hint["source_physical_page_count"] = 2
        hint["source_patched_page_count_equal"] = True
        self.assertEqual(validate_fake_result(record), record)

    def test_unverified_full_tail_shape_cannot_claim_success(self):
        record = ac1024_probe_record()
        for field in ("sampled_layer_eed_tail_shape_attempts",
                      "sampled_layer_eed_tail_shape_verified",
                      "sampled_layer_eed_tail_shape_h5_zero_length",
                      "sampled_layer_eed_tail_shape_token_count",
            "sampled_layer_eed_tail_shape_h5_payload",
            "sampled_layer_eed_tail_shape_non_h5_zero",
            "sampled_layer_eed_tail_shape_non_h5_payload",
            "sampled_layer_eed_tail_shape_total_bits",
            "sampled_layer_eed_tail_shape_refused_short_header",
            "sampled_layer_eed_tail_shape_refused_length",
            "sampled_layer_eed_tail_shape_refused_precondition",
            "sampled_layer_eed_terminal_fragment_verified",
            "sampled_layer_eed_terminal_fragment_zero",
            "sampled_layer_eed_terminal_fragment_bits_3",
            "sampled_layer_eed_terminal_fragment_tokens",
            "sampled_layer_eed_terminal_fragment_prefix_count",
            "sampled_layer_eed_terminal_fragment_prefix_code_5",
            "sampled_layer_eed_terminal_fragment_prefix_size_zero",
            "sampled_layer_eed_terminal_fragment_prefix_four_tokens",
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
            "sampled_layer_eed_terminal_candidate_ordinal_4_raw_ot_consensus"):
            invalid = deepcopy(record)
            invalid["ac1024_compatibility_hypothesis"][field] = 1
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_fake_result(invalid)

    def test_unverified_layer_tail_cannot_claim_source_success(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)
        for field in (
            "sampled_layer_eed_tail_header_attempts",
            "sampled_layer_eed_tail_header_verified",
            "sampled_layer_eed_tail_header_code_5",
            "sampled_layer_eed_tail_header_size_1",
        ):
            invalid = deepcopy(record)
            invalid["ac1024_compatibility_hypothesis"][field] = 1
            with self.subTest(field=field), self.assertRaises(ValueError):
                validate_fake_result(invalid)

    def test_zero_eed_evidence_is_bounded_and_never_geometry(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)
        for field, value in (
            ("sampled_zero_eed_attempts", 1),
            ("sampled_zero_eed_verified", 1),
            ("sampled_zero_eed_refused", 1),
            ("sampled_zero_eed_attempts", 2049),
            ("sampled_zero_eed_attempts", True),
            ("sampled_zero_eed_verified", -1),
        ):
            with self.subTest(field=field, value=value):
                invalid = deepcopy(record)
                invalid["ac1024_compatibility_hypothesis"][field] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)

    def test_no_proxy_cannot_exceed_crc_backed_zero_eed_subset(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for field, value in (
            ("sampled_no_proxy_verified", 1),
            ("sampled_no_proxy_refused", 1),
            ("sampled_no_proxy_verified", -1),
            ("sampled_no_proxy_refused", True),
        ):
            with self.subTest(field=field, value=value):
                invalid = deepcopy(original)
                invalid["ac1024_compatibility_hypothesis"][field] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)

    def test_entity_meta_subset_does_not_claim_owner_or_geometry(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for field, value in (
            ("sampled_entity_meta_attempts", 1),
            ("sampled_entity_meta_verified", 1),
            ("sampled_entity_meta_refused", 1),
            ("sampled_entity_meta_attempts", 2049),
            ("sampled_entity_meta_attempts", True),
            ("sampled_entity_meta_verified", -1),
        ):
            with self.subTest(field=field, value=value):
                invalid = deepcopy(original)
                invalid["ac1024_compatibility_hypothesis"][field] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)

    def test_owner_layer_sample_requires_verified_entity_and_recovered_index(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        # These are read-only reference witnesses, not geometry or display
        # acceptance. Missing, noninteger, negative, mismatched, or
        # unsubstantiated owner/layer counts must never pass.
        for field, value in (
            ("sampled_owner_layer_attempts", 1),
            ("sampled_owner_layer_verified", 1),
            ("sampled_owner_layer_refused", 1),
            ("sampled_owner_layer_attempts", 2049),
            ("sampled_owner_layer_attempts", True),
            ("sampled_owner_layer_verified", -1),
            ("sampled_owner_layer_refused", "0"),
        ):
            with self.subTest(field=field, value=value):
                invalid = deepcopy(original)
                invalid["ac1024_compatibility_hypothesis"][field] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)
        for missing in (
            "sampled_owner_layer_attempts",
            "sampled_owner_layer_verified",
            "sampled_owner_layer_refused",
        ):
            with self.subTest(missing=missing):
                invalid = deepcopy(original)
                del invalid["ac1024_compatibility_hypothesis"][missing]
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)

    def test_real_line_sample_counts_require_verified_record_provenance(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for field, value in (
            ("sampled_line_attempts", 1),
            ("sampled_line_common_verified", 1),
            ("sampled_line_common_refused", 1),
            ("sampled_line_xyz_verified", 1),
            ("sampled_line_xyz_refused", 1),
            ("sampled_line_attempts", 2049),
            ("sampled_line_attempts", True),
            ("sampled_line_common_verified", -1),
            ("sampled_line_xyz_refused", "0"),
            ("sampled_line_xyz_owner_layer_joined", 1),
            ("sampled_line_xyz_distinct_endpoints", 1),
            ("sampled_line_xyz_bounded_common", 1),
            ("sampled_line_xyz_bounded_common", True),
            ("sampled_line_xyz_owner_layer_joined", -1),
            ("sampled_line_meta_refused", 1),
            ("sampled_line_meta_self_refused", 1),
            ("sampled_line_meta_eed_refused", 1),
            ("sampled_line_meta_proxy_refused", 1),
            ("sampled_line_meta_entity_fields_refused", 1),
            ("sampled_line_eed_chain_attempts", 1),
            ("sampled_line_eed_chain_verified", 1),
            ("sampled_line_eed_chain_refused", 1),
            ("sampled_line_eed_chain_has_applications", 1),
            ("sampled_line_eed_chain_has_applications", True),
            ("sampled_line_eed_xyz_attempts", 1),
            ("sampled_line_eed_xyz_verified", 1),
            ("sampled_line_eed_xyz_refused", 1),
            ("sampled_line_eed_xyz_distinct_endpoints", 1),
            ("sampled_line_eed_xyz_attempts", True),
            ("sampled_line_eed_join_attempts", 1),
            ("sampled_line_eed_join_verified", 1),
            ("sampled_line_eed_join_refused", 1),
            ("sampled_line_eed_join_explicit_owner", 1),
            ("sampled_line_eed_join_implicit_layer", 1),
            ("sampled_line_eed_join_xdictionary", 1),
            ("sampled_line_eed_appid_attempts", 1),
            ("sampled_line_eed_appid_verified", 1),
            ("sampled_line_eed_appid_refused", 1),
            ("sampled_line_eed_appid_references_verified", 1),
            ("sampled_line_eed_appid_distinct_references_verified", 1),
            ("sampled_line_eed_appid_verified", True),
            ("sampled_layer_raw_flags_attempts", 1),
            ("sampled_layer_raw_flags_verified", 1),
            ("sampled_layer_raw_flags_refused", 1),
            ("sampled_layer_flags_refused_index", 1),
            ("sampled_layer_flags_refused_eed", 1),
            ("sampled_layer_flags_refused_record", 1),
            ("sampled_layer_flags_refused_reactors", 1),
            ("sampled_layer_flags_refused_xdic_bit", 1),
            ("sampled_layer_flags_refused_numeric", 1),
            ("sampled_layer_flags_refused_other", 1),
            ("sampled_layer_flags_refused_eed", True),
            ("sampled_layer_eed_flags_attempts", 1),
            ("sampled_layer_eed_flags_verified", 1),
            ("sampled_layer_eed_flags_refused", 1),
            ("sampled_layer_eed_flags_frozen_bit", 1),
            ("sampled_layer_eed_flags_on_bit", 1),
            ("sampled_layer_eed_flags_locked_bit", 1),
            ("sampled_layer_eed_flags_unmapped_bits", 1),
            ("sampled_layer_eed_flags_xdictionary_present", 1),
            ("sampled_layer_eed_flags_verified", True),
            ("sampled_layer_eed_appid_attempts", 1),
            ("sampled_layer_eed_appid_verified", 1),
            ("sampled_layer_eed_appid_refused", 1),
            ("sampled_layer_eed_appid_refs_verified", 1),
            ("sampled_layer_eed_xdic_graph_attempts", 1),
            ("sampled_layer_eed_xdic_graph_verified", 1),
            ("sampled_layer_eed_xdic_graph_refused", 1),
            ("sampled_layer_eed_xdic_refused_identity", 1),
            ("sampled_layer_eed_xdic_refused_eed", 1),
            ("sampled_layer_eed_xdic_refused_flags", 1),
            ("sampled_layer_eed_xdic_refused_appid", 1),
            ("sampled_layer_eed_xdic_refused_tail", 1),
            ("sampled_layer_eed_xdic_refused_control_h", 1),
            ("sampled_layer_eed_xdic_refused_dictionary_h", 1),
            ("sampled_layer_eed_xdic_refused_target_index", 1),
            ("sampled_layer_eed_xdic_refused_target_class", 1),
            ("sampled_layer_eed_xdic_refused_target_crc", 1),
            ("sampled_layer_eed_xdic_refused_other", 1),
            ("sampled_layer_eed_first_h_attempts", 1),
            ("sampled_layer_eed_first_h_verified", 1),
            ("sampled_layer_eed_first_h_refused", 1),
            ("sampled_layer_eed_first_h_code_0", 1),
            ("sampled_layer_eed_first_h_code_1", 1),
            ("sampled_layer_eed_first_h_code_2", 1),
            ("sampled_layer_eed_first_h_code_3", 1),
            ("sampled_layer_eed_first_h_code_4", 1),
            ("sampled_layer_eed_first_h_code_5", 1),
            ("sampled_layer_eed_first_h_code_6", 1),
            ("sampled_layer_eed_first_h_code_7", 1),
            ("sampled_layer_eed_first_h_code_8", 1),
            ("sampled_layer_eed_first_h_code_9", 1),
            ("sampled_layer_eed_first_h_code_10", 1),
            ("sampled_layer_eed_first_h_code_11", 1),
            ("sampled_layer_eed_first_h_code_12", 1),
            ("sampled_layer_eed_first_h_code_13", 1),
            ("sampled_layer_eed_first_h_code_14", 1),
            ("sampled_layer_eed_first_h_code_15", 1),
            ("sampled_layer_eed_first_h_size_0", 1),
            ("sampled_layer_eed_first_h_size_1", 1),
            ("sampled_layer_eed_first_h_size_2", 1),
            ("sampled_layer_eed_first_h_size_other", 1),
            ("sampled_layer_eed_first_h_code_2", True),
            ("sampled_layer_eed_xdic_refused_control_h", True),
            ("sampled_layer_eed_xdic_graph_has_tail", 1),
            ("sampled_layer_eed_xdic_graph_verified", True),
            ("sampled_layer_eed_appid_verified", True),
            ("sampled_layer_raw_flags_frozen_bit", 1),
            ("sampled_layer_raw_flags_on_bit", 1),
            ("sampled_layer_raw_flags_locked_bit", 1),
            ("sampled_layer_raw_flags_unmapped_bits", 1),
            ("sampled_layer_raw_flags_xdictionary_present", 1),
            ("sampled_layer_raw_flags_verified", True),
            ("sampled_line_eed_join_verified", True),
            ("sampled_line_common_after_meta_refused", 1),
            ("sampled_line_xyz_after_common_refused", 1),
            ("sampled_line_meta_refused", True),
            ("sampled_line_xyz_after_common_refused", -1),
            ("sampled_line_common_mode_unsupported", 1),
            ("sampled_line_common_reactors_unsupported", 1),
            ("sampled_line_common_xdic_unsupported", 1),
            ("sampled_line_common_fields_refused", 1),
            ("sampled_line_common_fields_refused", True),
            ("sampled_line_common_stage_color", 1),
            ("sampled_line_common_stage_rgb", 1),
            ("sampled_line_common_stage_transparency", 1),
            ("sampled_line_common_stage_ltype", 1),
            ("sampled_line_ltype_zero", 1),
            ("sampled_line_ltype_negative", 1),
            ("sampled_line_ltype_nonfinite", 1),
            ("sampled_line_ltype_parse_refused", 1),
            ("sampled_line_ltype_zero", True),
            ("sampled_line_common_stage_appearance", 1),
            ("sampled_line_common_stage_geom_bound", 1),
            ("sampled_line_common_stage_other", 1),
            ("sampled_line_common_stage_color", True),
            ("sampled_line_implicit_paper_xyz_verified", 1),
            ("sampled_line_implicit_model_xyz_verified", 1),
            ("sampled_line_implicit_model_xyz_verified", True),
            ("sampled_line_graph_attempts", 1),
            ("sampled_line_graph_verified", 1),
            ("sampled_line_graph_refused", 1),
            ("sampled_line_graph_verified", True),
            ("sampled_line_implicit_layer_attempts", 1),
            ("sampled_line_implicit_layer_verified", 1),
            ("sampled_line_implicit_layer_refused", 1),
            ("sampled_line_implicit_layer_attempts", True),
            ("sampled_line_xdic_present_attempts", 1),
            ("sampled_line_xdic_present_xyz_verified", 1),
            ("sampled_line_xdic_present_xyz_refused", 1),
            ("sampled_line_xdic_present_attempts", True),
            ("sampled_line_xdic_hgraph_attempts", 1),
            ("sampled_line_xdic_hgraph_verified", 1),
            ("sampled_line_xdic_hgraph_refused", 1),
            ("sampled_line_xdic_hgraph_verified", True),
        ):
            with self.subTest(field=field, value=value):
                invalid = deepcopy(original)
                invalid["ac1024_compatibility_hypothesis"][field] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)
        for missing in (
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
        ):
            with self.subTest(missing=missing):
                invalid = deepcopy(original)
                del invalid["ac1024_compatibility_hypothesis"][missing]
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)

    def test_candidate_handles_do_not_prove_entities(self):
        record = ac1024_probe_record()
        self.assertEqual(validate_fake_result(record), record)

    def test_reject_fake_original_decoder_acceptance(self):
        for mutation in (
            {"result": "accepted"},
            {"native_stage": "ac1024_crc_backed_handle_index"},
            {"evidence_count": 145279},
            {"geometry_verified": True},
        ):
            with self.subTest(mutation=mutation):
                record = ac1024_probe_record()
                record.update(mutation)
                with self.assertRaises(ValueError):
                    validate_fake_result(record)


    def test_page_count_evidence_requires_authenticated_section_pages(self):
        original = ac1024_probe_record()
        self.assertEqual(validate_fake_result(original), original)
        for field, invalid_value in (
            ("object_section_page_count", 0),
            ("object_section_page_count", -1),
            ("object_section_page_count", 70000),
            ("handle_section_page_count", 0),
            ("handle_section_page_count", "77"),
        ):
            with self.subTest(field=field):
                record = deepcopy(original)
                record["ac1024_compatibility_hypothesis"][field] = invalid_value
                with self.assertRaises(ValueError):
                    validate_fake_result(record)

    def test_reject_invalid_candidate_chain_or_claim(self):
        for key, value in (
            ("validated_original_ac1024_decoder", True),
            ("patched_signature_in_memory_only", False),
            ("physical_page_map", False),
            ("candidate_handle_count", 0),
            ("candidate_handle_count", -1),
            ("candidate_handle_index", "true"),
        ):
            with self.subTest(key=key, value=value):
                record = ac1024_probe_record()
                record["ac1024_compatibility_hypothesis"][key] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(record)


    def test_crc_frame_count_contract(self):
        record = ac1024_probe_record()
        hint = record["ac1024_compatibility_hypothesis"]
        hint["legacy_frame_probe_completed"] = True
        hint["legacy_frame_crc_ok"] = 145279
        hint["legacy_raw_type_19_line"] = 44941
        hint["legacy_raw_type_18_circle"] = 9543
        self.assertEqual(validate_fake_result(record), record)
        for field, value in (
            ("legacy_frame_crc_ok", 145278),
            ("legacy_frame_crc_failed", 1),
            ("legacy_raw_type_17_arc", 100000),
            ("legacy_frame_probe_completed", False),
        ):
            with self.subTest(field=field):
                corrupt = deepcopy(record)
                corrupt["ac1024_compatibility_hypothesis"][field] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(corrupt)


    def test_continuous_offset_candidate_only_and_consistent_crc_counts(self):
        record = ac1024_probe_record()
        hint = record["ac1024_compatibility_hypothesis"]
        hint["continuous_offset_candidate_index"] = True
        hint["continuous_candidate_handles"] = 145279
        hint["continuous_frame_crc_ok"] = 145000
        hint["continuous_frame_crc_failed"] = 279
        self.assertEqual(validate_fake_result(record), record)
        for key, value in (
            ("continuous_candidate_handles", 145278),
            ("continuous_frame_crc_ok", 145279),
            ("continuous_frame_crc_failed", -1),
            ("continuous_offset_candidate_index", False),
        ):
            with self.subTest(key=key, value=value):
                corrupt = deepcopy(record)
                corrupt["ac1024_compatibility_hypothesis"][key] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(corrupt)


    def test_page_absolute_candidate_requires_full_crc_accounting(self):
        record = ac1024_probe_record()
        hint = record["ac1024_compatibility_hypothesis"]
        hint["page_absolute_candidate_index"] = True
        hint["page_absolute_candidate_handles"] = 145279
        hint["page_absolute_frame_crc_ok"] = 120000
        hint["page_absolute_frame_crc_failed"] = 25279
        self.assertEqual(validate_fake_result(record), record)
        for key, value in (
            ("page_absolute_candidate_handles", 145278),
            ("page_absolute_frame_crc_ok", 145279),
            ("page_absolute_frame_crc_failed", -1),
            ("page_absolute_candidate_index", False),
        ):
            with self.subTest(key=key):
                corrupt = deepcopy(record)
                corrupt["ac1024_compatibility_hypothesis"][key] = value
                with self.assertRaises(ValueError):
                    validate_fake_result(corrupt)


    def test_duplicate_offset_evidence_is_not_a_decoded_object_count(self):
        record = ac1024_probe_record()
        hint = record["ac1024_compatibility_hypothesis"]
        hint.update({
            "legacy_offset_distribution_measured": True,
            "legacy_distinct_offsets": 325,
            "legacy_duplicate_offset_entries": 144954,
            "legacy_zero_offset_entries": 1,
            "legacy_max_offset_multiplicity": 1300,
            "legacy_distinct_frame_crc_pass": 2,
            "legacy_repeated_crc_pass_entries": 558,
            "legacy_frame_too_short": 0,
            "legacy_frame_invalid_ms": 100000,
            "legacy_frame_exceeds_section": 10000,
            "legacy_frame_crc_mismatch": 34721,
            "legacy_shift_minus_two_crc_pass": 4,
            "legacy_shift_minus_one_crc_pass": 8,
            "legacy_shift_plus_one_crc_pass": 6,
            "legacy_shift_plus_two_crc_pass": 5,
            "legacy_shift_samples_tested": 128,
            "legacy_next_offset_pairs": 324,
            "legacy_next_frame_unavailable": 0,
            "legacy_next_frame_exact": 10,
            "legacy_next_frame_short": 314,
            "legacy_next_frame_overlaps": 0,
            "legacy_crc_ok_next_frame_exact": 1,
            "legacy_short_gap_over64": 314,
            "mc_frame_probe_completed": True,
            "mc_frame_crc_pass": 100,
            "mc_frame_crc_mismatch": 145177,
            "mc_frame_unreadable": 2,
            "mc_frame_next_exact": 100,
            "mc_byte_len_matches_short_gap": 100,
            "mc_bytes_1": 130000,
            "mc_bytes_2": 15277,
            "mc_ot_decoded": 100,
            "mc_ot_unreadable": 0,
            "mc_ot_raw_19": 25,
            "core_frame_verified": 100,
            "core_payload_bounds_verified": 100,
            "core_stream_bounds_verified": 99,
            "core_stream_bounds_rejected": 1,
            "sampled_first_h_attempts": 100,
            "sampled_first_h_self_matches": 75,
            "sampled_first_h_other": 2,
            "sampled_first_h_unreadable": 23,
            "sampled_first_h_code0_mismatch": 2,
            "sampled_first_h_reverse_bytes_matches": 1,
            "sampled_first_h_in_handle_index": 76,
            "sampled_first_h_not_in_handle_index": 1,
            "sampled_first_h_same_index_offset": 75,
            "sampled_first_h_count2": 50,
            "sampled_first_h_count3": 20,
            "sampled_first_h_count4to8": 7,
            "sampled_scan_best_shift_bits": 0,
            "sampled_scan_best_self_matches": 75,
            "sampled_h_internally_distinct": 70,
            "sampled_h_internal_duplicates": 7,
            "sampled_h_successive_pairs": 76,
            "sampled_h_successive_increases": 70,
            "sampled_h_successive_decreases": 3,
            "sampled_h_successive_equals": 3,
            "sampled_h_less_than_index": 1,
            "sampled_h_greater_than_index": 1,
            "sampled_h_index_requires_over3_bytes": 0,
            "sampled_h_low8_equal_index": 75,
            "sampled_h_low16_equal_index": 75,
            "full_embedded_h_census_completed": True,
            "full_embedded_h_attempts": 145279,
            "full_embedded_h_parsed_code0": 145277,
            "full_embedded_h_other_codes": 0,
            "full_embedded_h_unreadable": 2,
            "full_embedded_h_self_matches": 2000,
            "full_embedded_h_less_than_index": 143277,
            "full_embedded_h_greater_than_index": 0,
            "full_embedded_h_neighbor_pairs": 145274,
            "full_embedded_h_increases": 145274,
            "full_embedded_h_decreases": 0,
            "full_embedded_h_equals": 0,
            "core_embedded_h_recovery_attempted": True,
            "mc_unreadable_invalid_ms": 2,
            "mc_unreadable_first_half": 1,
            "mc_invalid_ms_zero": 2,
        })
        self.assertEqual(validate_fake_result(record), record)
        self.assertEqual(record["result"], "not_covered_by_this_native_probe")
        self.assertFalse(record["geometry_verified"])
        for field, bad_value in (
            ("legacy_distinct_offsets", 326),
            ("legacy_duplicate_offset_entries", 144953),
            ("legacy_zero_offset_entries", 1400),
            ("legacy_max_offset_multiplicity", 0),
            ("legacy_distinct_frame_crc_pass", 326),
            ("legacy_repeated_crc_pass_entries", 1),
            ("legacy_offset_distribution_measured", False),
            ("legacy_frame_crc_mismatch", 1),
            ("legacy_shift_plus_one_crc_pass", 145280),
            ("legacy_shift_samples_tested", 0),
            ("legacy_next_offset_pairs", 323),
            ("legacy_next_frame_exact", 324),
            ("legacy_crc_ok_next_frame_exact", 11),
            ("legacy_next_frame_unavailable", 100001),
            ("legacy_short_gap_over64", 1),
            ("mc_frame_crc_pass", 1),
            ("mc_frame_next_exact", 145279),
            ("mc_byte_len_matches_short_gap", 315),
            ("mc_bytes_1", 1),
            ("mc_frame_probe_completed", False),
            ("mc_ot_decoded", 101),
            ("mc_ot_raw_19", 101),
            ("core_frame_verified", 99),
            ("core_frame_probe_failed", 1),
            ("core_raw_ot_or_span_mismatch", 1),
            ("core_payload_bounds_verified", 99),
            ("core_payload_bounds_rejected", 1),
            ("core_payload_bounds_mismatch", 1),
            ("mc_unreadable_invalid_ms", 1),
            ("mc_unreadable_first_half", 3),
            ("mc_invalid_ms_zero", 1),
            ("mc_invalid_ms_unclassified", 1),
            ("core_stream_bounds_verified", 98),
            ("core_stream_bounds_mismatch", 1),
            ("sampled_first_h_attempts", 2049),
            ("sampled_first_h_self_matches", 120),
            ("sampled_first_h_code0_mismatch", 1),
            ("sampled_first_h_reverse_bytes_matches", 3),
            ("sampled_first_h_in_handle_index", 75),
            ("sampled_first_h_not_in_handle_index", 3),
            ("sampled_first_h_same_index_offset", 77),
            ("sampled_first_h_count2", 78),
            ("sampled_scan_best_shift_bits", 33),
            ("sampled_scan_best_self_matches", 74),
            ("sampled_h_internally_distinct", 69),
            ("sampled_h_successive_pairs", 77),
            ("sampled_h_successive_increases", 71),
            ("sampled_h_less_than_index", 2),
            ("sampled_h_index_requires_over3_bytes", 78),
            ("sampled_h_low16_equal_index", 74),
            ("full_embedded_h_attempts", 145278),
            ("full_embedded_h_parsed_code0", 145278),
            ("full_embedded_h_neighbor_pairs", 145277),
            ("full_embedded_h_less_than_index", 143276),
            ("core_embedded_h_recovery_attempted", False),
            ("core_embedded_h_recovery_succeeded", True),
            ("core_embedded_h_recovery_records", 145279),
        ):
            with self.subTest(field=field):
                invalid = deepcopy(record)
                invalid["ac1024_compatibility_hypothesis"][field] = bad_value
                with self.assertRaises(ValueError):
                    validate_fake_result(invalid)

    def test_unique_offsets_cannot_claim_repeated_records(self):
        record = ac1024_probe_record()
        hint = record["ac1024_compatibility_hypothesis"]
        hint.update({
            "legacy_offset_distribution_measured": True,
            "legacy_distinct_offsets": 145279,
            "legacy_duplicate_offset_entries": 0,
            "legacy_zero_offset_entries": 1,
            "legacy_max_offset_multiplicity": 1,
            "legacy_distinct_frame_crc_pass": 300,
            "legacy_repeated_crc_pass_entries": 300,
            "legacy_frame_too_short": 0,
            "legacy_frame_invalid_ms": 140000,
            "legacy_frame_exceeds_section": 4000,
            "legacy_frame_crc_mismatch": 979,
            "legacy_shift_samples_tested": 2048,
            "legacy_next_offset_pairs": 145278,
            "legacy_next_frame_unavailable": 2,
            "legacy_next_frame_exact": 0,
            "legacy_next_frame_short": 145276,
            "legacy_next_frame_overlaps": 0,
            "legacy_crc_ok_next_frame_exact": 0,
            "legacy_short_gap_2": 145276,
            "mc_frame_probe_completed": True,
            "mc_frame_crc_pass": 145277,
            "mc_frame_crc_mismatch": 0,
            "mc_frame_unreadable": 2,
            "mc_frame_next_exact": 145276,
            "mc_byte_len_matches_short_gap": 145276,
            "mc_bytes_1": 135600,
            "mc_bytes_2": 9600,
            "mc_bytes_3": 77,
            "mc_ot_decoded": 145277,
            "mc_ot_unreadable": 0,
            "mc_ot_raw_7": 31799,
            "mc_ot_raw_19": 44941,
            "mc_ot_raw_49": 659,
            "mc_ot_raw_51": 175,
            "mc_ot_raw_77": 13987,
            "core_frame_verified": 145277,
            "core_payload_bounds_verified": 145277,
            "core_stream_bounds_verified": 145277,
            "sampled_first_h_attempts": 2048,
            "sampled_first_h_self_matches": 2000,
            "sampled_first_h_other": 5,
            "sampled_first_h_unreadable": 43,
            "sampled_first_h_code0_mismatch": 5,
            "sampled_first_h_reverse_bytes_matches": 2,
            "sampled_first_h_in_handle_index": 2002,
            "sampled_first_h_not_in_handle_index": 3,
            "sampled_first_h_same_index_offset": 2000,
            "sampled_first_h_count2": 1000,
            "sampled_first_h_count3": 700,
            "sampled_first_h_count4to8": 305,
            "sampled_scan_best_shift_bits": 0,
            "sampled_scan_best_self_matches": 2000,
            "sampled_h_internally_distinct": 2000,
            "sampled_h_internal_duplicates": 5,
            "sampled_h_successive_pairs": 2004,
            "sampled_h_successive_increases": 1995,
            "sampled_h_successive_decreases": 5,
            "sampled_h_successive_equals": 4,
            "sampled_h_less_than_index": 2,
            "sampled_h_greater_than_index": 3,
            "sampled_h_index_requires_over3_bytes": 0,
            "sampled_h_low8_equal_index": 2000,
            "sampled_h_low16_equal_index": 2000,
            "full_embedded_h_census_completed": True,
            "full_embedded_h_attempts": 145279,
            "full_embedded_h_parsed_code0": 145277,
            "full_embedded_h_other_codes": 0,
            "full_embedded_h_unreadable": 2,
            "full_embedded_h_self_matches": 2000,
            "full_embedded_h_less_than_index": 143277,
            "full_embedded_h_greater_than_index": 0,
            "full_embedded_h_neighbor_pairs": 145274,
            "full_embedded_h_increases": 145274,
            "full_embedded_h_decreases": 0,
            "full_embedded_h_equals": 0,
            "core_embedded_h_recovery_attempted": True,
            "mc_unreadable_invalid_ms": 2,
            "mc_unreadable_first_half": 1,
            "mc_invalid_ms_zero": 2,
        })
        self.assertEqual(validate_fake_result(record), record)
        hint["masked6_candidate_index_valid"] = True
        hint["masked6_candidate_handle_count"] = 145279
        hint["masked6_sample_attempts"] = 2048
        hint["masked6_sample_self_matches"] = 0
        hint["masked6_sample_other"] = 2048
        hint["masked6_sample_unreadable"] = 0
        self.assertEqual(validate_fake_result(record), record)
        invalid_alt = deepcopy(record)
        invalid_alt["ac1024_compatibility_hypothesis"][
            "masked6_sample_attempts"] = 2047
        with self.assertRaises(ValueError):
            validate_fake_result(invalid_alt)
        hint["masked6_candidate_index_valid"] = False
        with self.assertRaises(ValueError):
            validate_fake_result(record)
        hint["masked6_candidate_index_valid"] = True
        hint["legacy_max_offset_multiplicity"] = 2
        with self.assertRaises(ValueError):
            validate_fake_result(record)

    def test_hint_never_applies_to_another_dwg_version(self):
        record = ac1024_probe_record()
        record["signature"] = "AC1032"
        with self.assertRaises(ValueError):
            validate_fake_result(record)


if __name__ == "__main__":
    unittest.main()
