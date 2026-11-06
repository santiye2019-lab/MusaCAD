"""Negative tests for real rejected-map diagnostic; no CAD admission."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

EXE = os.environ.get("MUSACAD_REJECTED_MAP_DIAGNOSTIC", "")


@unittest.skipUnless(EXE, "native diagnostic executable required by CI")
class RejectedMapDiagnosticTest(unittest.TestCase):
    def probe(self, source: bytes):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "test.dwg"
            path.write_bytes(source)
            before = path.read_bytes()
            process = subprocess.run(
                [EXE, str(path)], capture_output=True, text=True,
                timeout=30, check=True,
            )
            self.assertEqual(path.read_bytes(), before)
            return json.loads(process.stdout)

    def test_fake_2004_header_never_promoted(self):
        row = self.probe(b"AC1018" + bytes(512))
        self.assertEqual(row["diagnostic"], "authenticated_header_rejected")

    def test_fake_2018_header_never_promoted(self):
        row = self.probe(b"AC1032" + bytes(512))
        self.assertEqual(row["diagnostic"], "authenticated_header_rejected")

    def test_r2010_fake_header_not_promoted(self):
        row = self.probe(b"AC1024" + bytes(512))
        self.assertEqual(row["diagnostic"], "authenticated_header_rejected")

    def test_uncovered_version_not_accepted(self):
        row = self.probe(b"AC1027" + bytes(512))
        self.assertEqual(row["diagnostic"], "unsupported_signature")

    def test_short_source_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "truncated.dwg"
            path.write_bytes(b"AC")
            process = subprocess.run([EXE, str(path)], capture_output=True)
            self.assertNotEqual(process.returncode, 0)


if __name__ == "__main__":
    unittest.main()
