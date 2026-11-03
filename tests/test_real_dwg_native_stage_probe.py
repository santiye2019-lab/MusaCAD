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

    def test_r2010_version_is_not_falsely_accepted_as_r2004(self):
        record = self.probe(b"AC1024" + bytes(506))
        self.assertEqual(record["result"], "not_covered_by_this_native_probe")
        self.assertEqual(record["native_stage"], "ac1024_family_hypothesis_only")
        self.assertEqual(record["evidence_count"], 0)
        hypothesis = record["ac1024_compatibility_hypothesis"]
        self.assertTrue(hypothesis["patched_signature_in_memory_only"])
        self.assertFalse(hypothesis["validated_original_ac1024_decoder"])
        for key in ("page_map_header", "physical_page_map",
                    "objects_section_pages", "handles_section_pages",
                    "candidate_handle_index"):
            self.assertFalse(hypothesis[key], key)
        self.assertEqual(hypothesis["candidate_handle_count"], 0)

    def test_r2013_version_is_not_falsely_accepted_as_r2018(self):
        record = self.probe(b"AC1027" + bytes(506))
        self.assertEqual(record["result"], "not_covered_by_this_native_probe")
        self.assertEqual(record["native_stage"], "signature_only")

    def test_unknown_ascii_six_byte_header_rejected(self):
        record = self.probe(b"AC9999" + bytes(506))
        self.assertEqual(record["result"], "rejected")
        self.assertEqual(record["native_stage"], "invalid_signature")


if __name__ == "__main__":
    unittest.main()
