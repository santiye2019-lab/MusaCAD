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
            "mc_unreadable_invalid_ms": 2,
            "mc_unreadable_first_half": 1,
            "mc_invalid_ms_zero": 2,
        })
        self.assertEqual(validate_fake_result(record), record)
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
