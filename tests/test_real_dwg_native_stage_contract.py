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

    def test_hint_never_applies_to_another_dwg_version(self):
        record = ac1024_probe_record()
        record["signature"] = "AC1032"
        with self.assertRaises(ValueError):
            validate_fake_result(record)


if __name__ == "__main__":
    unittest.main()
