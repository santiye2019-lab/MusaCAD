"""Negative tests for the real DWG LFS integrity gate (no real DWGs needed)."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

REPO = Path(__file__).resolve().parent.parent
AUDITOR = REPO / "tests" / "real_dwg_lfs_corpus_audit.py"


def command(*argv, cwd=None):
    return subprocess.run(argv, cwd=cwd, check=True, capture_output=True)


class RealDwgLfsCorpusAuditTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="musacad-dwg-test-")
        cls.library = Path(cls.build.name) / "libmusacad_dwgsig.so"
        command(
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-fPIC", "-shared", "-I" + str(REPO / "core/native/include"),
            str(REPO / "core/native/src/musacad_dwg_signature.c"),
            "-o", str(cls.library),
        )

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def setUp(self):
        self.work = tempfile.TemporaryDirectory(prefix="musacad-audit-case-")
        self.root = Path(self.work.name)
        command("git", "init", "-q", cwd=self.root)
        command("git", "config", "user.email", "test@example.invalid", cwd=self.root)
        command("git", "config", "user.name", "Fixture Test", cwd=self.root)
        (self.root / "Projeler").mkdir()

    def tearDown(self):
        self.work.cleanup()

    def fixture(self, binary, materialized=True):
        drawing = self.root / "Projeler" / "example.dwg"
        pointer = (
            "version https://git-lfs.github.com/spec/v1\n"
            "oid sha256:%s\n"
            "size %d\n"
        ) % (hashlib.sha256(binary).hexdigest(), len(binary))
        drawing.write_text(pointer, encoding="ascii")
        command("git", "add", "Projeler/example.dwg", cwd=self.root)
        command("git", "commit", "-q", "-m", "pointer fixture", cwd=self.root)
        if materialized:
            drawing.write_bytes(binary)
        return drawing

    def audit(self):
        report = self.root / "report.json"
        run = subprocess.run(
            [sys.executable, str(AUDITOR), "--repository", str(self.root),
             "--native-sig-library", str(self.library),
             "--report", str(report)],
            capture_output=True, text=True,
        )
        return run.returncode, json.loads(report.read_text(encoding="utf-8"))

    def test_hydrated_valid_lfs_and_native_header(self):
        self.fixture(b"AC1032" + bytes(1024))
        code, result = self.audit()
        self.assertEqual(code, 0)
        self.assertEqual(result["passed"], 1)
        self.assertEqual(result["version_counts"], {"AC1032": 1})
        self.assertIn("does_not_verify", result)

    def test_unhydrated_pointer_fails(self):
        self.fixture(b"AC1032" + bytes(1024), materialized=False)
        code, result = self.audit()
        self.assertNotEqual(code, 0)
        self.assertEqual(result["failed"], 1)

    def test_sha256_mismatch_fails(self):
        file = self.fixture(b"AC1015" + bytes(1024))
        file.write_bytes(b"AC1015" + bytes(1023) + b"x")
        code, result = self.audit()
        self.assertNotEqual(code, 0)
        self.assertIn("lfs_sha256_mismatch", result["files"][0]["error"])

    def test_unknown_header_is_not_accepted(self):
        self.fixture(b"AC9999" + bytes(1024))
        code, result = self.audit()
        self.assertNotEqual(code, 0)
        self.assertEqual(
            result["files"][0]["error"], "unsupported_or_invalid_dwg_signature"
        )

    def test_empty_corpus_fails(self):
        command("git", "commit", "--allow-empty", "-q", "-m", "empty", cwd=self.root)
        code, result = self.audit()
        self.assertNotEqual(code, 0)
        self.assertEqual(result["dwg_file_count"], 0)


if __name__ == "__main__":
    unittest.main()
