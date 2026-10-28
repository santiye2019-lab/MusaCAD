#!/usr/bin/env python3
"""Conservative MusaCAD2 source boundary guard; not a legal provenance audit."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CODE_SUFFIXES = {".c", ".h", ".cc", ".cpp", ".hpp", ".cmake", ".gradle", ".kt", ".java", ".swift"}
SOURCE_DIRS = ("core", "tests")
FORBIDDEN = (
    (re.compile(r"SPDX-License-Identifier:\s*(?:GPL|AGPL|LGPL)(?:-|\b)", re.I), "copyleft SPDX marker"),
    (re.compile(r"GNU (?:AFFERO )?GENERAL PUBLIC LICENSE", re.I), "GPL/AGPL license declaration"),
    (re.compile(r"\b(?:libredwg|libre_dwg)\b", re.I), "LibreDWG reference"),
    (re.compile(r"#\s*include\s*[<\"]\s*(?:\.\./|core/native/|musacad_)", re.I), "legacy include dependency"),
)
def main():
    failures = []
    for folder in SOURCE_DIRS:
        for path in (ROOT / folder).rglob("*"):
            if not path.is_file() or path.suffix.lower() not in CODE_SUFFIXES:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for pattern, reason in FORBIDDEN:
                if pattern.search(text):
                    failures.append(f"{path.relative_to(ROOT)}: {reason}")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for pattern, reason in FORBIDDEN:
        if pattern.search(cmake):
            failures.append(f"CMakeLists.txt: {reason}")
    if failures:
        print("MusaCAD2 source boundary FAILED:\n" + "\n".join(failures), file=sys.stderr)
        return 1
    print("MusaCAD2 source boundary scan passed (heuristic only; provenance review still required).")
    return 0
if __name__ == "__main__":
    raise SystemExit(main())
