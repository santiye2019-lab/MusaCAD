#!/usr/bin/env python3
"""Read-only original DWG versus exported DXF ACIS transport regression.

An entity count is NOT proof of B-Rep/tessellation fidelity. This gate checks
that a DWG with original ACIS entities did not lose entity records or their
SAT/SAB payload in the native LibreDWG conversion. It never edits the DWG.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys


SOURCE_LINE = re.compile(r"MUSACAD_ACIS_SOURCE=(\d+) EXTERNAL=(\d+)")
ACIS_TYPES = {"3DSOLID", "REGION", "BODY"}
BYTE_CAP = 16 * 1024 * 1024


class AcisAuditError(ValueError):
    pass


def source_counts(log: Path) -> tuple[int, int]:
    text = log.read_text(encoding="utf-8", errors="replace")
    match = SOURCE_LINE.search(text)
    if not match:
        raise AcisAuditError("Native DWG source inventory is missing")
    source, external = map(int, match.groups())
    if external > source:
        raise AcisAuditError("Impossible source external AcDs inventory")
    return source, external


def decode_sat(text: str) -> str:
    text = text.replace("^ ", "^")
    return "".join(chr(159 - ord(c)) if 32 < ord(c) < 159 else c for c in text)


def sat_markers(text: str) -> bool:
    """Structural hints only; no claim that faces or surfaces are valid."""
    variants = (text, decode_sat(text))
    for value in variants:
        lower = value.lower()
        if "end-of-acis" in lower and re.search(
            r"\b(body|face|edge|vertex|shell|lump|point|surface)\b", lower
        ):
            return True
    return False


def audit(path: Path, source: int, external: int) -> dict:
    if not path.is_file():
        raise AcisAuditError(f"Missing converted DXF: {path}")
    section = ""
    needs_section_name = False
    current_type = ""
    text_parts: list[str] = []
    binary_chunks: list[str] = []
    buffered = 0
    found = 0
    missing = 0
    malformed = 0
    sat_hints = 0
    sab_hints = 0

    def finish_entity() -> None:
        nonlocal found, missing, malformed, sat_hints, sab_hints, buffered
        if current_type not in ACIS_TYPES:
            return
        found += 1
        text = " ".join(text_parts)
        has_text = any(text_parts)
        has_binary = any(binary_chunks)
        binary_valid = all(
            len(value) % 2 == 0 and re.fullmatch(r"[0-9a-fA-F]+", value)
            for value in binary_chunks
        )
        if has_binary and not binary_valid:
            malformed += 1
        if not has_text and not has_binary:
            missing += 1
        if has_text and sat_markers(text):
            sat_hints += 1
        if has_binary and binary_valid:
            # A valid hex block is only binary payload presence, not ACIS proof.
            sab_hints += 1

    with path.open("r", encoding="latin-1", newline="") as stream:
        while True:
            code_line = stream.readline()
            if not code_line:
                break
            value_line = stream.readline()
            if not value_line:
                raise AcisAuditError("Incomplete DXF group code/value pair")
            try:
                code = int(code_line.strip())
            except ValueError as exc:
                raise AcisAuditError(f"Invalid DXF group code: {code_line!r}") from exc
            value = value_line.rstrip("\r\n")
            if needs_section_name:
                if code != 2:
                    raise AcisAuditError("Missing DXF section name")
                section = value.upper()
                needs_section_name = False
                continue
            if code == 0:
                finish_entity()
                text_parts.clear()
                binary_chunks.clear()
                buffered = 0
                current_type = ""
                tag = value.upper()
                if tag == "SECTION":
                    needs_section_name = True
                elif tag == "ENDSEC":
                    section = ""
                elif tag in ACIS_TYPES and section in ("BLOCKS", "ENTITIES"):
                    current_type = tag
            elif current_type and code in (1, 3, 310):
                buffered += len(value)
                if buffered > BYTE_CAP:
                    raise AcisAuditError("ACIS payload audit exceeded bounded entity budget")
                if code == 310:
                    binary_chunks.append(value)
                else:
                    text_parts.append(value)
        finish_entity()

    return {
        "source_entities": source,
        "source_external_acds": external,
        "exported_entities": found,
        "missing_payload_entities": missing,
        "malformed_hex_entities": malformed,
        "sat_structural_hints": sat_hints,
        "hex_payload_entities": sab_hints,
    }


def verify(report: dict, require_source: bool = False) -> None:
    source = report["source_entities"]
    exported = report["exported_entities"]
    if require_source and source == 0:
        raise AcisAuditError("Fixture has no source ACIS entity: invalid regression sample")
    if source > 0 and exported < source:
        raise AcisAuditError(f"Source DWG has {source} ACIS entities; DXF has only {exported}")
    if source > 0 and report["missing_payload_entities"] > 0:
        raise AcisAuditError(
            f"{report['missing_payload_entities']} exported ACIS entities lack SAT/SAB data"
        )
    if report["malformed_hex_entities"]:
        raise AcisAuditError("Exported ACIS body has invalid binary hex")
    # Do not require tessellation: unsupported legitimate ACIS surfaces may exist.
    # But for a concrete source fixture, empty export MUST NOT pass.
    if source > 0 and not (report["sat_structural_hints"] or report["hex_payload_entities"]):
        raise AcisAuditError("No exported SAT record structure / SAB binary payload detected")


def self_test() -> None:
    import tempfile

    def dxf(*entities: str) -> str:
        return "0\nSECTION\n2\nENTITIES\n" + "".join(entities) + "0\nENDSEC\n0\nEOF\n"

    sat = "700 0 1 0 body $-1 # End-of-ACIS-data"
    encrypted = decode_sat(sat)
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / "fixture.dxf"
        path.write_text(
            dxf("0\n3DSOLID\n1\n" + encrypted + "\n"),
            encoding="latin-1",
        )
        good = audit(path, 1, 0)
        assert good["sat_structural_hints"] == 1 and good["exported_entities"] == 1
        verify(good, require_source=True)
        path.write_text(dxf("0\nREGION\n70\n2\n"), encoding="latin-1")
        try:
            verify(audit(path, 1, 1), require_source=True)
        except AcisAuditError:
            pass
        else:
            raise AssertionError("Missing AcDs body was incorrectly accepted")
        path.write_text(dxf("0\nBODY\n310\n41Z0\n"), encoding="latin-1")
        try:
            verify(audit(path, 1, 0), require_source=True)
        except AcisAuditError:
            pass
        else:
            raise AssertionError("Corrupt SAB body was incorrectly accepted")
    print("PASS: ACIS native source-versus-DXF payload loss gate")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("dxf", nargs="?", type=Path)
    p.add_argument("--source-log", type=Path)
    p.add_argument("--require-source", action="store_true")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()
    try:
        if args.self_test:
            self_test()
        if args.dxf:
            if not args.source_log:
                raise AcisAuditError("--source-log is required for DWG comparison")
            report = audit(args.dxf, *source_counts(args.source_log))
            print(json.dumps(report, sort_keys=True))
            verify(report, require_source=args.require_source)
        elif not args.self_test:
            raise AcisAuditError("Supply an exported DXF or --self-test")
    except (AcisAuditError, OSError) as exc:
        print("FAIL: " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
