#!/usr/bin/env python3
"""Conservative MusaCAD2 source boundary guard; not a legal provenance audit."""
from pathlib import Path
import re
import csv
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
    ledger_path = ROOT / "migration" / "TRANSFER_LEDGER.csv"
    with ledger_path.open(encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    required = {"legacy_path", "new_path", "source_commit", "authors", "rights_evidence", "dependencies", "classification", "decision", "tests", "status"}
    if not rows or not required.issubset(rows[0].keys()):
        failures.append("migration/TRANSFER_LEDGER.csv: missing mandatory provenance columns")
    else:
        ledger = {}
        for row in rows:
            name = row["new_path"].strip()
            if not name or name in ledger:
                failures.append(f"migration ledger: empty or duplicate path {name!r}")
            ledger[name] = row
        for folder in ("core/include", "core/src"):
            for path in (ROOT / folder).rglob("*"):
                if path.is_file() and path.suffix.lower() in CODE_SUFFIXES:
                    rel = path.relative_to(ROOT).as_posix()
                    row = ledger.get(rel)
                    if not row:
                        failures.append(f"{rel}: missing provenance ledger entry")
                    elif not all(row.get(key, "").strip() for key in ("authors", "rights_evidence", "dependencies", "classification", "decision", "status")):
                        failures.append(f"{rel}: incomplete provenance ledger entry")
                    elif row["legacy_path"].strip() and (
                        row["decision"].strip() != "APPROVE" or
                        row["status"].strip() not in ("VERIFIED", "TESTED") or
                        not row["source_commit"].strip()
                    ):
                        failures.append(f"{rel}: legacy transfer not approved with source revision")
    if failures:
        print("MusaCAD2 source boundary FAILED:\n" + "\n".join(failures), file=sys.stderr)
        return 1
    print("MusaCAD2 source boundary scan passed (heuristic only; provenance review still required).")
    return 0
if __name__ == "__main__":
    raise SystemExit(main())
