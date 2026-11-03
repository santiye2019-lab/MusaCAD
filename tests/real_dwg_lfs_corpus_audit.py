#!/usr/bin/env python3
"""Read-only integrity gate for real DWG Git LFS objects in Projeler/.

This verifies materialized LFS bytes, Git-recorded object digest/size and the
portable MusaCAD C signature classifier. It DOES NOT parse or certify geometry,
layers, blocks, color, viewport visibility or DWG write support.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
from collections import Counter

SIGNATURE_CODES = {
    b"AC1009": 1,
    b"AC1012": 2,
    b"AC1014": 3,
    b"AC1015": 4,
    b"AC1018": 5,
    b"AC1021": 6,
    b"AC1024": 7,
    b"AC1027": 8,
    b"AC1032": 9,
}
LFS_POINTER = re.compile(
    rb"version https://git-lfs.github.com/spec/v1\r?\n"
    rb"oid sha256:([a-f0-9]{64})\r?\n"
    rb"size ([0-9]+)\r?\n?"
)


def git_bytes(root: Path, *args: str) -> bytes:
    return subprocess.run(
        ["git", *args], cwd=root, check=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ).stdout


def tracked_dwgs(root: Path) -> list[str]:
    paths = git_bytes(root, "ls-files", "-z", "--", "Projeler").split(b"\x00")
    return sorted(
        p.decode("utf-8") for p in paths if p
        and p.decode("utf-8").lower().endswith(".dwg")
    )


def file_digest(path: Path) -> tuple[str, int, bytes]:
    digest = hashlib.sha256()
    count = 0
    first_six = b""
    with path.open("rb") as source:
        while True:
            block = source.read(1024 * 1024)
            if not block:
                break
            if not first_six:
                first_six = block[:6]
            digest.update(block)
            count += len(block)
    return digest.hexdigest(), count, first_six


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=Path, default=Path("."))
    parser.add_argument("--native-sig-library", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    root = args.repository.resolve()
    library = ctypes.CDLL(str(args.native_sig_library.resolve()))
    identify = library.musa_core_dwg_identify_signature
    identify.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    identify.restype = ctypes.c_int

    records = []
    versions = Counter()
    files = tracked_dwgs(root)
    for relative in files:
        record = {"path": relative, "status": "fail"}
        records.append(record)
        try:
            target = root / relative
            if target.is_symlink() or not target.is_file():
                raise ValueError("missing_or_symlinked_file")
            pointer = git_bytes(root, "show", "HEAD:" + relative)
            match = LFS_POINTER.fullmatch(pointer)
            if match is None:
                raise ValueError("not_an_expected_git_lfs_pointer")
            expected_digest = match.group(1).decode("ascii")
            expected_bytes = int(match.group(2))
            if expected_bytes < 256:
                raise ValueError("unexpectedly_short_dwg")
            actual_digest, actual_bytes, header = file_digest(target)
            record["byte_count"] = actual_bytes
            record["source_sha256"] = expected_digest
            record["dwg_signature"] = header.decode("ascii", errors="replace")
            if actual_bytes != expected_bytes:
                raise ValueError("lfs_size_mismatch_or_unhydrated_pointer")
            if actual_digest != expected_digest:
                raise ValueError("lfs_sha256_mismatch")
            if header not in SIGNATURE_CODES:
                raise ValueError("unsupported_or_invalid_dwg_signature")
            six = ctypes.create_string_buffer(header, 6)
            if identify(six, 6) != SIGNATURE_CODES[header]:
                raise ValueError("native_c_signature_disagreement")
            record["status"] = "lfs_bytes_and_native_signature_verified"
            versions[record["dwg_signature"]] += 1
        except (OSError, ValueError, subprocess.CalledProcessError) as exc:
            record["error"] = str(exc)
    passed = sum(item["status"] != "fail" for item in records)
    report = {
        "check": "real_dwg_lfs_readonly_header_integrity_only",
        "scope": "LFS byte integrity and native C six-byte version detection",
        "does_not_verify": [
            "geometry", "layers", "colors", "blocks", "ownership",
            "coordinates", "rendering", "XREF", "DWG writing",
        ],
        "dwg_file_count": len(files),
        "passed": passed,
        "failed": len(files) - passed,
        "version_counts": dict(sorted(versions.items())),
        "files": records,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print("Real DWG LFS audit: %d/%d passed; signatures=%s" %
          (passed, len(files), dict(sorted(versions.items()))))
    for item in records:
        if item["status"] == "fail":
            print("FAIL %s: %s" % (item["path"], item.get("error", "")),
                  file=sys.stderr)
    if not files:
        print("FAIL no tracked DWG files in Projeler", file=sys.stderr)
    print("Header/LFS integrity is NOT DWG geometry or rendering acceptance.")
    return 0 if files and passed == len(files) else 1


if __name__ == "__main__":
    sys.exit(main())
