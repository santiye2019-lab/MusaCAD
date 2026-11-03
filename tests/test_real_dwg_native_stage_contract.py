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

    def test_hint_never_applies_to_another_dwg_version(self):
        record = ac1024_probe_record()
        record["signature"] = "AC1032"
        with self.assertRaises(ValueError):
            validate_fake_result(record)


if __name__ == "__main__":
    unittest.main()
