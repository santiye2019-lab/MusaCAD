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


if __name__ == "__main__":
    unittest.main()
