#!/usr/bin/env python3
"""Strict DXF-record parity gate for a FUTURE AC1032 writer.

This is an output-side acceptance primitive, NOT a DWG encoder or an
independent check against the original DWG. The caller must first export
two DXF inventories through independent readers of (1) the approved edited
model and (2) the freshly written, independently reopened AC1032 binary.

Compare all ordered code/value pairs of audited DXF records, plus critical
HEADER variables (units, dimension scales, drawing base and active layer).
This catches
silent changes to geometry, layers, color, text, handles, blocks and XREF
references even when entity counts, extents and filenames are identical.
Entity order may change; order of group codes/vertices inside an entity MUST
stay unchanged. Unsupported modern/proxy/ACIS entities FAIL CLOSED because
identical exported DXF views cannot prove binary semantics for these classes.

This bounded gate never modifies inputs and never publishes files.
"""
from __future__ import annotations

import argparse
from collections import Counter
from decimal import Decimal, InvalidOperation
from hashlib import sha256
import json
from pathlib import Path

MAX_DXF_BYTES = 128 * 1024 * 1024
MAX_RECORDS = 250_000
MAX_GROUP_PAIRS = 4_000_000
MAX_PAIRS_PER_RECORD = 100_000
AUDITED_SECTIONS = frozenset(("TABLES", "BLOCKS", "ENTITIES", "OBJECTS", "CLASSES"))
SKIPPED_SECTIONS = frozenset(("THUMBNAILIMAGE",))
UNVERIFIABLE_SECTIONS = frozenset(("ACDSDATA",))
CRITICAL_HEADER_VARS = frozenset((
    "$INSUNITS", "$MEASUREMENT", "$LUNITS", "$LUPREC", "$AUNITS",
    "$AUPREC", "$ANGBASE", "$ANGDIR", "$INSBASE", "$CLAYER",
    "$CELTYPE", "$CECOLOR", "$CELWEIGHT", "$CELTSCALE",
    "$LTSCALE", "$PSLTSCALE", "$DIMSCALE", "$DIMLFAC",
    "$DIMLUNIT", "$DIMAUNIT", "$DIMDEC", "$DIMZIN",
    "$UCSORG", "$UCSXDIR", "$UCSYDIR", "$DWGCODEPAGE",
))
UNVERIFIABLE_ENTITY_CLASSES = frozenset((
    "3DSOLID", "BODY", "REGION", "ACAD_PROXY_ENTITY", "ACAD_PROXY_OBJECT",
    "ACAD_TABLE", "ACSH_BREP_CLASS", "ACSH_BOOLEAN_CLASS",
))
SUPPORTED_SECTIONS = AUDITED_SECTIONS | SKIPPED_SECTIONS | UNVERIFIABLE_SECTIONS | {"HEADER"}
STRUCTURAL = frozenset(("SECTION", "ENDSEC", "EOF"))
HEX_HANDLE_CODES = frozenset((5, 105)) | frozenset(range(320, 370)) | frozenset(range(390, 400))
INT_CODES = (
    frozenset(range(60, 100)) | frozenset(range(160, 180)) |
    frozenset(range(270, 300)) | frozenset(range(370, 390)) |
    frozenset(range(400, 410)) | frozenset(range(420, 460)) |
    frozenset(range(1060, 1072))
)
FLOAT_CODES = (
    frozenset(range(10, 60)) | frozenset(range(110, 150)) |
    frozenset(range(210, 240)) | frozenset(range(460, 470)) |
    frozenset(range(1010, 1060))
)


class DxfParityError(ValueError):
    pass


def _normalized(code: int, value: str) -> str:
    """Canonical numeric text only; retain exact strings and record order."""
    if code in FLOAT_CODES:
        try:
            number = Decimal(value)
        except InvalidOperation as error:
            raise DxfParityError("invalid numeric DXF coordinate") from error
        if not number.is_finite():
            raise DxfParityError("nonfinite DXF numeric field")
        # Refuse exponent bombs: a short "1e999999999" must not allocate
        # gigabytes when canonicalized to a fixed-point decimal string.
        if abs(number.as_tuple().exponent) > 512 or len(number.as_tuple().digits) > 10000:
            raise DxfParityError("out-of-budget DXF numeric field")
        if number == 0:
            return "0"
        # Decimal.normalize() rounds to the current process context
        # precision (typically 28 digits). That could accidentally make
        # two distinct high-precision CAD coordinates compare equal.
        # Canonicalize via the exact coefficient and exponent instead.
        triplet = number.as_tuple()
        coefficient = "".join(str(d) for d in triplet.digits).lstrip("0")
        exponent = triplet.exponent
        while coefficient.endswith("0"):
            coefficient = coefficient[:-1]
            exponent += 1
        if exponent >= 0:
            literal = coefficient + "0" * exponent
        else:
            point = len(coefficient) + exponent
            literal = (
                coefficient[:point] + "." + coefficient[point:]
                if point > 0 else "0." + "0" * (-point) + coefficient
            )
        return ("-" if triplet.sign else "") + literal
    if code in INT_CODES:
        try:
            return str(int(value, 10))
        except ValueError as error:
            raise DxfParityError("invalid integer DXF field") from error
    if code in HEX_HANDLE_CODES:
        candidate = value.strip().upper()
        if not candidate or any(c not in "0123456789ABCDEF" for c in candidate):
            raise DxfParityError("invalid CAD handle / reference")
        return candidate.lstrip("0") or "0"
    return value


def fingerprint(path: Path) -> dict:
    """Read-only semantic multiset; never reconstructs, alters or publishes."""
    if not path.is_file() or not 1 <= path.stat().st_size <= MAX_DXF_BYTES:
        raise DxfParityError("DXF missing or outside bounded file size")
    sections = set()
    records = Counter()
    counts = Counter()
    unverified = Counter()
    pair_count = 0
    record_count = 0
    section = ""
    kind = None
    attrs: list[tuple[int, str]] = []
    has_eof = False

    def fingerprint_header(attrs: list[tuple[int, str]]) -> None:
        """Capture critical DXF HEADER $VARIABLE groups, not volatile timestamps.

        Values in HEADER follow a 9/$NAME marker inside the SECTION record.
        Capture ordered typed group codes to detect units, dimensions, drawing
        insertion base, active layer or world UCS silently changing.
        """
        var = None
        values: list[tuple[int, str]] = []
        seen = set()

        def store() -> None:
            if var not in CRITICAL_HEADER_VARS:
                return
            if var in seen:
                raise DxfParityError("duplicate critical DXF HEADER variable")
            seen.add(var)
            if not values:
                raise DxfParityError("empty critical DXF HEADER variable")
            h = sha256()
            for code, value in values:
                h.update(str(code).encode("ascii") + b"\x00")
                h.update(value.encode("utf-8") + b"\x00")
            records[("HEADER", var, h.hexdigest())] += 1
            counts[("HEADER", var)] += 1

        for code, raw in attrs:
            if code == 9:
                store()
                var = raw.strip().upper()
                values = []
            elif var in CRITICAL_HEADER_VARS and code != 999:
                values.append((code, _normalized(code, raw)))
        store()

    def finish() -> None:
        nonlocal section, record_count, has_eof
        if kind is None:
            return
        if kind == "SECTION":
            if section:
                raise DxfParityError("nested DXF SECTION")
            names = [v.upper() for k, v in attrs if k == 2]
            if len(names) != 1 or names[0] not in SUPPORTED_SECTIONS:
                raise DxfParityError("unrecognized DXF SECTION")
            section = names[0]
            if section in sections:
                raise DxfParityError("duplicate DXF SECTION")
            sections.add(section)
            if section == "HEADER":
                fingerprint_header(attrs)
            if section in UNVERIFIABLE_SECTIONS:
                unverified[section] += 1
            return
        if kind == "ENDSEC":
            if not section or attrs:
                raise DxfParityError("invalid DXF ENDSEC or hidden section payload")
            section = ""
            return
        if kind == "EOF":
            if section or has_eof or attrs:
                raise DxfParityError("invalid DXF EOF or hidden trailing payload")
            has_eof = True
            return
        if has_eof:
            raise DxfParityError("records after DXF EOF")
        if not section:
            raise DxfParityError("entity outside DXF SECTION")
        if section in SKIPPED_SECTIONS:
            return
        if section in UNVERIFIABLE_SECTIONS:
            return
        record_count += 1
        if record_count > MAX_RECORDS:
            raise DxfParityError("too many DXF records")
        if kind in UNVERIFIABLE_ENTITY_CLASSES:
            unverified[kind] += 1
        h = sha256()
        h.update(section.encode("ascii") + b"\x00" + kind.encode("utf-8") + b"\x00")
        for code, raw in attrs:
            if code == 999:
                continue  # DXF comments are not drawing semantics
            val = _normalized(code, raw)
            h.update(str(code).encode("ascii") + b"\x00")
            h.update(val.encode("utf-8") + b"\x00")
        digest = h.hexdigest()
        records[(section, kind, digest)] += 1
        counts[(section, kind)] += 1

    with path.open("r", encoding="utf-8", errors="strict", newline=None) as inp:
        while True:
            code_line = inp.readline()
            if not code_line:
                break
            value_line = inp.readline()
            if not value_line:
                raise DxfParityError("incomplete DXF group code/value")
            try:
                code = int(code_line.strip())
            except ValueError as error:
                raise DxfParityError("invalid ASCII DXF group code") from error
            value = value_line.rstrip("\r\n")
            pair_count += 1
            if pair_count > MAX_GROUP_PAIRS:
                raise DxfParityError("DXF field limit exceeded")
            if code == 0:
                finish()
                if has_eof:
                    raise DxfParityError("DXF content follows EOF")
                kind = value.upper()
                attrs = []
            else:
                if kind is None:
                    raise DxfParityError("DXF group outside entity record")
                attrs.append((code, value))
                if len(attrs) > MAX_PAIRS_PER_RECORD:
                    raise DxfParityError("oversized DXF record")
        finish()
    if not has_eof or section or "ENTITIES" not in sections:
        raise DxfParityError("DXF missing EOF, ENTITIES or ENDSEC")
    if not counts or not any(k[0] in ("ENTITIES", "BLOCKS") for k in counts):
        raise DxfParityError("DXF has no drawing entities")
    return {
        "records": records,
        "counts": counts,
        "unverified": unverified,
        "record_count": record_count,
        "pair_count": pair_count,
        "sections": sorted(sections),
    }


def compare(approved_dxf: Path, reopened_dxf: Path) -> dict:
    source = fingerprint(approved_dxf)
    result = fingerprint(reopened_dxf)
    missing = source["records"] - result["records"]
    extra = result["records"] - source["records"]
    unverified = source["unverified"] + result["unverified"]
    # HEADER variables can be entirely dropped while the DXF geometry and
    # entity count stay unchanged. The presence of the HEADER section itself
    # must agree, even if neither export listed our selected variables.
    header_mismatch = ("HEADER" in source["sections"]) != ("HEADER" in result["sections"])
    identical = not missing and not extra and not header_mismatch
    passed = identical and not unverified
    # Counts and digests ONLY in diagnostics: never print customer drawing
    # coordinates, text, layer names or raw entity attributes.
    return {
        "pass": passed,
        "scope": "exact audited DXF record parity; not an independent original DWG/ACIS proof",
        "approved_records": source["record_count"],
        "reopened_records": result["record_count"],
        "missing_records": sum(missing.values()),
        "extra_records": sum(extra.values()),
        "critical_header_mismatch": header_mismatch or any(
            section == "HEADER" for section, _, _ in missing | extra
        ),
        "unverifiable_entities": sum(unverified.values()),
        "reason": ("pass" if passed else
                   "unsupported CAD/proxy/ACIS semantics" if unverified else
                   "DXF geometry or attribute parity mismatch"),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("approved_dxf", type=Path)
    ap.add_argument("reopened_dxf", type=Path)
    ap.add_argument("--report", type=Path)
    args = ap.parse_args()
    try:
        row = compare(args.approved_dxf, args.reopened_dxf)
    except (OSError, UnicodeError, DxfParityError) as error:
        row = {"pass": False, "reason": type(error).__name__,
               "scope": "fail-closed DXF-only semantic gate"}
    if args.report:
        args.report.write_text(json.dumps(row, indent=2) + "\n", encoding="utf-8")
    print(("PASS" if row["pass"] else "FAIL") +
          " proposed AC1032 DXF semantic check: " + json.dumps(row))
    return 0 if row["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
