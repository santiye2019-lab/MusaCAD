"""Host-native regression: no geometry or container acceptance from a signature."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

BIN = os.environ.get("MUSACAD_NATIVE_STAGE_PROBE", "")


@unittest.skipUnless(BIN, "MUSACAD_NATIVE_STAGE_PROBE is required in CI")
class NativeDwgStageProbeTest(unittest.TestCase):
    def probe(self, content: bytes) -> dict:
        with tempfile.TemporaryDirectory(prefix="musacad-dwg-probe-") as folder:
            path = Path(folder) / "fixture.dwg"
            path.write_bytes(content)
            before = path.read_bytes()
            result = subprocess.run(
                [BIN, str(path)], check=True, capture_output=True,
                text=True, timeout=20,
            )
            after = path.read_bytes()
            self.assertEqual(before, after, "source bytes must be immutable")
            record = json.loads(result.stdout)
            self.assertFalse(record["geometry_verified"])
            self.assertFalse(record["rendering_verified"])
            self.assertFalse(record["source_modified"])
            return record

    def test_r2000_fake_container_does_not_pass_crc(self):
        record = self.probe(b"AC1015" + bytes(506))
        self.assertEqual(record["result"], "rejected")
        self.assertEqual(record["native_stage"], "ac1015_section_locators")

    def test_r2004_fake_container_does_not_pass_handle_index(self):
        record = self.probe(b"AC1018" + bytes(506))
        self.assertEqual(record["result"], "rejected")
        self.assertEqual(record["native_stage"], "ac1018_page_map_header")

    def test_r2018_fake_container_does_not_pass_page_map(self):
        record = self.probe(b"AC1032" + bytes(506))
        self.assertEqual(record["result"], "rejected")
        self.assertEqual(record["native_stage"], "ac1032_page_map_header")

    def test_r2010_fake_container_does_not_pass_protected_page_map(self):
        record = self.probe(b"AC1024" + bytes(506))
        self.assertEqual(record["result"], "rejected")
        self.assertEqual(record["native_stage"], "ac1024_page_map_header")

    def test_r2013_version_is_not_falsely_accepted_as_r2018(self):
        record = self.probe(b"AC1027" + bytes(506))
        self.assertEqual(record["result"], "not_covered_by_this_native_probe")
        self.assertEqual(record["native_stage"], "signature_only")

    def test_unknown_ascii_six_byte_header_rejected(self):
        record = self.probe(b"AC9999" + bytes(506))
        self.assertEqual(record["result"], "rejected")
        self.assertEqual(record["native_stage"], "invalid_signature")



class NativeStageContractTest(unittest.TestCase):
    """Do not report index-only AC1018 as verified record coverage."""

    def run_mocked_stage(self, signature: str, stage: str,
                         status: str = "accepted") -> dict:
        from types import SimpleNamespace
        from unittest.mock import patch
        from real_dwg_native_stage_matrix import run_probe

        record = {
            "signature": signature,
            "result": status,
            "native_stage": stage,
            "evidence_count": 3,
            "geometry_verified": False,
            "rendering_verified": False,
            "source_modified": False,
            "ac1024_section_map_crc_verified": False,
            "ac1024_section_map_lz_completed": False,
            "ac1024_directory_shape_candidate": False,
            "ac1024_directory_page_refs_candidate": 0,
            "ac1024_directory_physical_links_candidate": False,
            "ac1024_directory_matched_physical_refs_candidate": 0,
            "ac1024_data_masked_headers_verified": 0,
            "ac1024_data_payload_adler_verified": 0,
            "ac1024_data_header_adler_verified": 0,
            "ac1024_data_lz_eligible_pages": 0,
            "ac1024_data_lz_verified": 0,
            "ac1024_handles_join_verified": False,
            "ac1024_handles_join_logical_bytes": 0,
            "ac1024_handles_join_probe_reason": 0,
            "ac1024_handles_join_inner_failure_stage": 0,
            "ac1024_objects_join_verified": False,
            "ac1024_objects_join_logical_bytes": 0,
            "ac1024_objects_candidate_logical_bytes": 0,
            "ac1024_objects_join_inner_failure_stage": 0,
            "ac1024_handles_crc_frames_verified": False,
            "ac1024_handles_crc_data_pages": 0,
            "ac1024_handles_crc_page_data_bytes": 0,
            "ac1024_handle_candidate_profiles": 0,
            "ac1024_handle_candidate_pairs": 0,
            "ac1024_handle_crc_self_prefix_pairs": 0,
            "ac1024_handle_candidate_numeric_matches": 0,
            "ac1024_handle_candidate_profile": 0,
            "ac1024_handle_complete_self_index_verified": False,
            "ac1024_handle_complete_numeric_agreement": False,
        }
        fake = SimpleNamespace(returncode=0, stdout=json.dumps(record))
        with patch("real_dwg_native_stage_matrix.subprocess.run",
                   return_value=fake) as subprocess_mock:
            result = run_probe(Path("/mock/probe"), Path("/mock/test.dwg"), 1)
            subprocess_mock.assert_called_once()
        return result

    def test_ac1018_index_only_acceptance_is_refused(self):
        with self.assertRaisesRegex(
                ValueError, "ac1018_index_only_is_not_full_record_acceptance"):
            self.run_mocked_stage("AC1018", "ac1018_crc_backed_handle_index")

    def test_ac1018_full_record_audit_acceptance_is_allowed(self):
        record = self.run_mocked_stage(
            "AC1018", "ac1018_record_ranges_and_crcs")
        self.assertEqual(record["evidence_count"], 3)

    def test_ac1018_early_rejection_remains_diagnostic(self):
        record = self.run_mocked_stage(
            "AC1018", "ac1018_crc_backed_handle_index", "rejected")
        self.assertEqual(record["result"], "rejected")

    def test_ac1024_header_only_acceptance_is_refused(self):
        with self.assertRaisesRegex(
                ValueError, "ac1024_header_only_is_not_map_acceptance"):
            self.run_mocked_stage("AC1024", "ac1024_page_map_header")

    def test_ac1024_original_map_stage_is_allowed(self):
        record = self.run_mocked_stage(
            "AC1024", "ac1024_physical_page_locations")
        self.assertEqual(record["evidence_count"], 3)

    def test_other_existing_stage_contract_is_not_changed(self):
        record = self.run_mocked_stage(
            "AC1032", "ac1032_crc_backed_handle_index")
        self.assertEqual(record["native_stage"],
                         "ac1032_crc_backed_handle_index")


class OriginalAc1024LogicalJoinGateTest(unittest.TestCase):
    def test_corpus_logical_join_must_be_real_not_header_only(self):
        from real_dwg_native_stage_matrix import require_original_ac1024_handles_join
        example = {
            "result": "accepted",
            "native_stage": "ac1024_physical_page_locations",
            "ac1024_handles_join_verified": True,
            "ac1024_handles_join_logical_bytes": 2048,
            "ac1024_handles_join_inner_failure_stage": 0,
            "geometry_verified": False,
            "rendering_verified": False,
        }
        require_original_ac1024_handles_join("AC1024", example)
        for key, bad in (
            ("ac1024_handles_join_verified", False),
            ("ac1024_handles_join_logical_bytes", 0),
            ("ac1024_handles_join_inner_failure_stage", 4),
            ("geometry_verified", True),
        ):
            bad_record = dict(example)
            bad_record[key] = bad
            with self.subTest(key=key), self.assertRaisesRegex(
                    ValueError, "original_ac1024_handles_logical_join_not_verified"):
                require_original_ac1024_handles_join("AC1024", bad_record)
        # AC1018, AC1032 and unsupported signatures are not assigned the
        # AC1024-specific Handles acceptance by a cross-version fallback.
        require_original_ac1024_handles_join("AC1032", {})


class OriginalAc1024ObjectsJoinGateTest(unittest.TestCase):
    def test_corpus_objects_join_is_not_silent_optional_evidence(self):
        from real_dwg_native_stage_matrix import require_original_ac1024_objects_join
        verified = {
            "result": "accepted",
            "native_stage": "ac1024_physical_page_locations",
            "ac1024_handles_join_verified": True,
            "ac1024_objects_join_verified": True,
            "ac1024_objects_join_logical_bytes": 12345,
            "ac1024_objects_candidate_logical_bytes": 12345,
            "ac1024_objects_join_inner_failure_stage": 0,
            "geometry_verified": False,
            "rendering_verified": False,
        }
        require_original_ac1024_objects_join("AC1024", verified)
        for key, value in (
            ("ac1024_objects_join_verified", False),
            ("ac1024_objects_join_logical_bytes", 0),
            ("ac1024_objects_candidate_logical_bytes", 12346),
            ("ac1024_objects_join_inner_failure_stage", 14),
            ("ac1024_handles_join_verified", False),
            ("geometry_verified", True),
        ):
            changed = dict(verified)
            changed[key] = value
            with self.subTest(field=key), self.assertRaisesRegex(
                    ValueError, "original_ac1024_objects_logical_join_not_verified"):
                require_original_ac1024_objects_join("AC1024", changed)
        require_original_ac1024_objects_join("AC1018", {})


class OriginalAc1024HandlesFrameGateTest(unittest.TestCase):
    def test_corpus_requires_original_crc_terminal_and_exact_byte_coverage(self):
        from real_dwg_native_stage_matrix import require_original_ac1024_handles_crc_frames
        verified = {
            "result": "accepted",
            "ac1024_handles_crc_frames_verified": True,
            "ac1024_handles_crc_data_pages": 2,
            "ac1024_handles_crc_page_data_bytes": 5,
            "ac1024_handles_join_logical_bytes": 17,
            "ac1024_handles_join_verified": True,
            "ac1024_objects_join_verified": True,
            "geometry_verified": False,
            "rendering_verified": False,
        }
        require_original_ac1024_handles_crc_frames("AC1024", verified)
        for key, value in (
            ("ac1024_handles_crc_frames_verified", False),
            ("ac1024_handles_crc_data_pages", 0),
            ("ac1024_handles_crc_data_pages", 3),
            ("ac1024_handles_crc_page_data_bytes", 6),
            ("ac1024_handles_join_logical_bytes", 18),
            ("ac1024_objects_join_verified", False),
            ("geometry_verified", True),
        ):
            malformed = dict(verified)
            malformed[key] = value
            with self.subTest(field=key), self.assertRaisesRegex(
                    ValueError, "original_ac1024_handles_crc_frames_not_verified"):
                require_original_ac1024_handles_crc_frames("AC1024", malformed)
        require_original_ac1024_handles_crc_frames("AC1018", {})


if __name__ == "__main__":
    unittest.main()
