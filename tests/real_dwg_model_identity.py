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


def parse_dxf(path):
    section = None
    kind = None
    groups = []
    seen = set()
    entities = {}

    def complete():
        nonlocal section, kind
        if kind == "SECTION":
            code2 = [v for c, v in groups if c == 2]
            if len(code2) != 1:
                raise ValueError("DXF section header invalid (group 2 count=%d, tag count=%d)" % (len(code2), len(groups)))
            section = code2[0].upper()
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
    return entities


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


def file_sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()


def verify(converter, original):
    if not converter.is_file() or not original.is_file():
        raise ValueError("native converter or DWG file missing")
    before = file_sha(original)
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
            target = parse_dxf(converted)
        except (ValueError, UnicodeError) as exc:
            raise ValueError("DXF model index invalid: " + str(exc)) from None
        count = compare_records(source, target)
    if file_sha(original) != before:
        raise ValueError("read-only DWG source changed")
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--converter", required=True, type=Path)
    parser.add_argument("dwg", type=Path)
    args = parser.parse_args()
    try:
        count = verify(args.converter.resolve(), args.dwg.resolve())
    except (ValueError, OSError, subprocess.SubprocessError) as exc:
        # No IDs, source drawing names, geometry or raw records are disclosed.
        # Parser messages describe counts/categories only; never expose handles,
        # coordinates, filenames or original engineering drawing payloads.
        reason = str(exc) if isinstance(exc, ValueError) else type(exc).__name__
        print("FAIL: original DWG / converted DXF identity: " + reason)
        return 1
    print("PASS: %d original DWG root entities retain handle+type in DXF "
          "(NOT independent AutoCAD parity)" % count)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
