#!/usr/bin/env python3
"""Read-only DXF structural/content gate for pinned LibreDWG conversions.

A file containing just SECTION/ENTITIES/EOF is not proof that CAD entities
survived conversion. This audit counts actual entity records and validates
DXF pair boundaries. It deliberately never silently marks unsupported DWG
object types as successfully converted; separate DWG-to-DXF coverage tests
are still required for that claim.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import sys


class DxfAuditError(ValueError):
    pass


def audit(path: Path) -> dict:
    if not path.is_file() or path.stat().st_size < 60:
        raise DxfAuditError(f"{path}: empty/very short DXF")
    section = None
    expect_section_name = False
    sections = Counter()
    entities = Counter()
    block_names = set()
    insert_names = Counter()
    entity_colors = Counter()
    entity_handles = set()
    entity_count = 0
    saw_eof = False
    lines = 0
    with path.open("r", encoding="utf-8", errors="replace", newline="") as stream:
        while True:
            code_line = stream.readline()
            if not code_line:
                break
            lines += 1
            code_line = code_line.strip().lstrip("\ufeff")
            if not code_line:
                # One trailing blank line is harmless, but blank tags mid-file
                # would shift the code/value pairing and corrupt the drawing.
                if not stream.read(1):
                    break
                raise DxfAuditError(f"{path}: blank DXF group code at line {lines}")
            value_line = stream.readline()
            if not value_line:
                raise DxfAuditError(f"{path}: incomplete group pair at line {lines}")
            lines += 1
            try:
                code = int(code_line)
            except ValueError as error:
                raise DxfAuditError(
                    f"{path}: invalid group code {code_line!r} at line {lines - 1}"
                ) from error
            value = value_line.strip()
            if saw_eof:
                raise DxfAuditError(f"{path}: content after EOF")
            if expect_section_name:
                if code != 2 or not value:
                    raise DxfAuditError(f"{path}: SECTION is missing name (group 2)")
                section = value.upper()
                sections[section] += 1
                expect_section_name = False
                continue
            if code == 0:
                name = value.upper()
                if name == "SECTION":
                    if section is not None:
                        raise DxfAuditError(f"{path}: nested SECTION ({section})")
                    expect_section_name = True
                elif name == "ENDSEC":
                    if section is None:
                        raise DxfAuditError(f"{path}: ENDSEC outside SECTION")
                    section = None
                elif name == "EOF":
                    if section is not None:
                        raise DxfAuditError(f"{path}: EOF inside {section}")
                    saw_eof = True
                elif section == "ENTITIES":
                    entities[name] += 1
                    entity_count += 1
                continue
            if section == "BLOCKS" and code == 2:
                # Includes block names and other group-2 tags; diagnostic only.
                block_names.add(value)
            if section == "ENTITIES":
                if code == 2 and entities and value:
                    insert_names[value] += 1  # candidate references, not a fidelity claim
                elif code in (62, 420):
                    entity_colors[str(code)] += 1
                elif code == 5 and value:
                    entity_handles.add(value.upper())

    if expect_section_name or section is not None or not saw_eof:
        raise DxfAuditError(f"{path}: incomplete SECTION/ENDSEC/EOF structure")
    if not sections["ENTITIES"]:
        raise DxfAuditError(f"{path}: missing ENTITIES section")
    if not entity_count:
        raise DxfAuditError(f"{path}: ENTITIES exists but contains no CAD objects")
    return {
        "file": str(path),
        "size_bytes": path.stat().st_size,
        "entity_records": entity_count,
        "entity_types": dict(sorted(entities.items())),
        "entity_handles": len(entity_handles),
        "color_tags": dict(sorted(entity_colors.items())),
        "sections": dict(sorted(sections.items())),
        "text_lines": lines,
    }


def self_test() -> None:
    import tempfile
    sample = (
        "0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\nA1\n8\nPIPE\n10\n1\n20\n2\n"
        "0\nTEXT\n5\nA2\n1\nFLOW\n0\nENDSEC\n0\nEOF\n"
    )
    with tempfile.TemporaryDirectory() as temp:
        file = Path(temp) / "sample.dxf"
        file.write_text(sample, encoding="utf-8")
        result = audit(file)
        assert result["entity_records"] == 2
        assert result["entity_types"] == {"LINE": 1, "TEXT": 1}
        assert result["entity_handles"] == 2
        for bad in (
            sample.replace("0\nTEXT\n", "0\n"),
            sample.replace("0\nLINE\n5\nA1\n8\nPIPE\n10\n1\n20\n2\n0\nTEXT\n5\nA2\n1\nFLOW\n", ""),
            sample.replace("0\nEOF\n", ""),
        ):
            file.write_text(bad, encoding="utf-8")
            try:
                audit(file)
            except DxfAuditError:
                pass
            else:
                raise AssertionError("bad DXF was incorrectly accepted")
    print("PASS DXF structural audit self-test")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="*", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    if not args.files:
        return 0 if args.self_test else 2
    ok = True
    for filename in args.files:
        try:
            result = audit(filename)
            print(json.dumps(result, ensure_ascii=False, sort_keys=True))
        except (DxfAuditError, OSError) as error:
            print(f"FAIL: {error}", file=sys.stderr)
            ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
