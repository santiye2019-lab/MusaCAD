#!/usr/bin/env python3
"""Host CI check: compare original binary DWG model handles with converted DXF.

Only controlled test DWGs may be used in public CI; private files stay local.
Native handles are held in an exclusive 0600 temporary file and are NEVER
printed. This validates *reader/converter consistency*, NOT the independent
AutoCAD 2026 reference, block instance ancestry or modern DWG writing.
"""
import argparse
from collections import Counter
import hashlib
import math
from pathlib import Path
import re
import subprocess
import tempfile

SUPPORTED = {"LINE", "3DFACE", "TRACE", "SOLID"}
HEX = re.compile(r"[0-9A-Fa-f]{1,16}\Z")


def normalize(value):
    if not isinstance(value, str) or not HEX.fullmatch(value):
        raise ValueError("invalid model entity handle")
    number = int(value, 16)
    if not number:
        raise ValueError("zero model entity handle")
    return format(number, "X")


def parse_native_export(path):
    records = {}
    for row in path.read_text(encoding="ascii").splitlines():
        fields = row.split("\t")
        if len(fields) != 2:
            raise ValueError("invalid native entity record")
        key, kind = normalize(fields[0]), fields[1]
        if kind not in SUPPORTED | {"OTHER"} or key in records:
            raise ValueError("duplicate or invalid native model record")
        records[key] = kind
        if len(records) > 250000:
            raise ValueError("native model index exceeds strict budget")
    if not records:
        raise ValueError("empty native model identity index")
    return {key: val for key, val in records.items() if val in SUPPORTED}



def primitive_geometry(kind, groups):
    """Strict WCS vertex audit for direct LINE/3DFACE only.

    This deliberately does NOT certify layer/color, invisible edges, OCS,
    blocks, ACIS, or the binary DWG's independently decoded coordinates.
    """
    indexes = (0, 1) if kind == "LINE" else (0, 1, 2, 3)
    wanted = {base + offset for base in (10, 20, 30)
              for offset in indexes}
    values = {}
    for code, raw in groups:
        if code not in wanted:
            continue
        if code in values:
            raise ValueError("repeated DXF primitive coordinate group")
        try:
            number = float(raw)
        except ValueError:
            raise ValueError("invalid DXF primitive coordinate") from None
        if not math.isfinite(number):
            raise ValueError("nonfinite DXF primitive coordinate")
        values[code] = number

    def point(i):
        if 10 + i not in values or 20 + i not in values:
            raise ValueError("missing required DXF primitive XY coordinate")
        return (values[10 + i], values[20 + i], values.get(30 + i, 0.0))

    if kind == "LINE":
        return (point(0), point(1))
    first_three = (point(0), point(1), point(2))
    fourth_x, fourth_y = 13 in values, 23 in values
    if fourth_x != fourth_y:
        raise ValueError("partial DXF 3DFACE fourth vertex")
    # A triangular 3DFACE may omit the fourth point (same as third).
    return first_three + ((point(3) if fourth_x else first_three[2]),)


def compare_geometry(initial, exported):
    """Conservatively reject silent coordinate edits despite identical IDs.

    Tolerance covers floating-point ASCII/DWG/ASCII roundoff, not engineering
    drawing edits. No identifiers, measurements or private file paths in logs.
    """
    if not initial or initial.keys() != exported.keys():
        raise ValueError("three-way LINE/3DFACE geometry coverage mismatch")
    for key, shape in initial.items():
        other = exported[key]
        if len(shape) != len(other) or any(
            not math.isclose(a, b, rel_tol=1e-13, abs_tol=1e-9)
            for point_a, point_b in zip(shape, other)
            for a, b in zip(point_a, point_b)
        ):
            raise ValueError("three-way LINE/3DFACE coordinates differ")
    return len(initial)


def parse_dxf(path, with_geometry=False):
    section = None
    kind = None
    groups = []
    seen = set()
    entities = {}
    geometry = {}

    def complete():
        nonlocal section, kind
        if kind == "SECTION":
            # DXF HEADER values can themselves contain many code-2 tags.
            # Only the INITIAL (2, section-name) pair declares SECTION.
            if not groups or groups[0][0] != 2 or not groups[0][1]:
                raise ValueError("DXF section header missing initial group-2 name")
            section = groups[0][1].upper()
        elif kind == "ENDSEC":
            section = None
        elif section == "ENTITIES" and kind in SUPPORTED:
            codes = [v for c, v in groups if c == 5]
            if len(codes) != 1:
                raise ValueError("DXF direct entity missing unique group-5 handle")
            model = [v for c, v in groups if c == 67]
            if len(model) > 1 or (model and model[0] not in ("0", "1")):
                raise ValueError("DXF model/paper flag invalid")
            if model == ["1"]:
                return
            key = normalize(codes[0])
            if key in seen:
                raise ValueError("duplicate DXF direct model handle")
            seen.add(key)
            entities[key] = kind
            if with_geometry and kind in ("LINE", "3DFACE"):
                geometry[key] = primitive_geometry(kind, groups)

    # Large user DXFs are not needed here; the public-fixture CI inputs are
    # strictly bounded, and no bytes are logged or committed.
    if path.stat().st_size > 64 * 1024 * 1024:
        raise ValueError("test DXF exceeds 64-MiB fixture budget")
    with path.open("r", encoding="utf-8", errors="strict") as source:
        while True:
            code_line = source.readline()
            if not code_line:
                break
            value = source.readline()
            if not value:
                raise ValueError("truncated DXF group pair")
            try:
                code = int(code_line.strip())
            except ValueError as exc:
                raise ValueError("invalid DXF group code") from exc
            value = value.strip()
            if code == 0:
                if kind is not None:
                    complete()
                kind = value
                groups = []
            elif kind is not None:
                groups.append((code, value))
                if len(groups) > 500000:
                    raise ValueError("oversized DXF entity")
        if kind is not None:
            complete()
    return (entities, geometry) if with_geometry else entities


def compare_records(native, dxf):
    if not native or not dxf:
        raise ValueError("no comparable real binary DWG primitives")
    # Strict equality on the root entity handle AND type, not counts alone.
    if native != dxf:
        expected = Counter(native.values())
        actual = Counter(dxf.values())
        raise ValueError(
            "DWG/DXF direct model identity mismatch "
            "(native count=%d; DXF count=%d; native types=%s; DXF types=%s)"
            % (len(native), len(dxf), dict(expected), dict(actual))
        )
    return len(native)


def compare_three_way(initial, native, exported):
    """Do not accept synchronized reader/converter loss of original entities."""
    compare_records(native, exported)
    if initial != native:
        raise ValueError("initial DXF to binary DWG identity/type mismatch "
                         "(input=%d; native=%d)" % (len(initial), len(native)))
    if initial != exported:
        raise ValueError("initial DXF to exported DXF identity/type mismatch "
                         "(input=%d; output=%d)" % (len(initial), len(exported)))
    return len(initial)


def validate_expected_coverage(native, minimum_entities, minimum_faces):
    """Make test fixtures fail if both reader and converter omit 3D faces."""
    if minimum_entities < 1 or minimum_faces < 0:
        raise ValueError("invalid test fixture acceptance minimum")
    if len(native) < minimum_entities:
        raise ValueError("real binary DWG identity fixture coverage too small")
    if sum(kind == "3DFACE" for kind in native.values()) < minimum_faces:
        raise ValueError("real binary DWG fixture lost required 3DFACE")
    return len(native)


def file_sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()


def verify(converter, original, minimum_entities=1, minimum_faces=0,
           source_dxf=None):
    if not converter.is_file() or not original.is_file():
        raise ValueError("native converter or DWG file missing")
    before = file_sha(original)
    input_checksum = None
    expected = None
    original_geometry = None
    if source_dxf is not None:
        if not source_dxf.is_file():
            raise ValueError("initial DXF source fixture missing")
        input_checksum = file_sha(source_dxf)
        try:
            expected, original_geometry = parse_dxf(source_dxf, with_geometry=True)
        except (ValueError, UnicodeError) as exc:
            raise ValueError("initial DXF model index invalid: " + str(exc)) from None
        validate_expected_coverage(expected, minimum_entities, minimum_faces)
    with tempfile.TemporaryDirectory(prefix="musa-real-dwg-id-") as tmp:
        temp = Path(tmp)
        model = temp / "model-identities.txt"
        converted = temp / "converted.dxf"
        try:
            subprocess.run(
                [str(converter), "--model-identity-probe", str(original), str(model)],
                check=True, timeout=90, stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
        except subprocess.SubprocessError:
            raise ValueError("native DWG root identity probe failed") from None
        if not model.is_file():
            raise ValueError("native model identities missing")
        try:
            source = parse_native_export(model)
        except ValueError as exc:
            raise ValueError("native DWG index validation: " + str(exc)) from None
        try:
            subprocess.run(
                [str(converter), str(original), str(converted)],
                check=True, timeout=90, stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
        except subprocess.SubprocessError:
            raise ValueError("DWG to DXF conversion failed") from None
        if not converted.is_file():
            raise ValueError("converted DXF missing")
        try:
            if expected is None:
                target = parse_dxf(converted)
                converted_geometry = None
            else:
                target, converted_geometry = parse_dxf(converted, with_geometry=True)
        except (ValueError, UnicodeError) as exc:
            raise ValueError("DXF model index invalid: " + str(exc)) from None
        count = compare_records(source, target)
        validate_expected_coverage(source, minimum_entities, minimum_faces)
        if expected is not None:
            count = compare_three_way(expected, source, target)
            # Identity/type parity is not sufficient if a LINE endpoint or
            # 3DFACE corner silently moves during the binary roundtrip.
            if original_geometry:
                compare_geometry(original_geometry, converted_geometry)
    if file_sha(original) != before:
        raise ValueError("read-only DWG source changed")
    if source_dxf is not None and file_sha(source_dxf) != input_checksum:
        raise ValueError("read-only initial DXF source changed")
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--converter", required=True, type=Path)
    parser.add_argument("--min-matched", type=int, default=1)
    parser.add_argument("--min-3dface", type=int, default=0)
    parser.add_argument("--source-dxf", type=Path,
                        help="Optional unchanged initial DXF, for three-way proof")
    parser.add_argument("dwg", type=Path)
    args = parser.parse_args()
    try:
        count = verify(args.converter.resolve(), args.dwg.resolve(),
                       minimum_entities=args.min_matched,
                       minimum_faces=args.min_3dface,
                       source_dxf=(args.source_dxf.resolve()
                                   if args.source_dxf is not None else None))
    except (ValueError, OSError, subprocess.SubprocessError) as exc:
        # No IDs, source drawing names, geometry or raw records are disclosed.
        # Parser messages describe counts/categories only; never expose handles,
        # coordinates, filenames or original engineering drawing payloads.
        reason = str(exc) if isinstance(exc, ValueError) else type(exc).__name__
        print("FAIL: original DWG / converted DXF identity: " + reason)
        return 1
    print("PASS: %d original DWG root entities retain handle+type in DXF%s "
          "(NOT independent AutoCAD parity)" % (
              count, " and initial source DXF" if args.source_dxf else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
