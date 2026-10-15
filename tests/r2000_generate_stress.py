#!/usr/bin/env python3
"""Deterministic host-only DXF stress fixtures for the guarded R2000 writer.

Produces synthetic test geometry; no customer drawing is uploaded or modified.
The pinned LibreDWG Line.dxf serves only as a well-formed DXF envelope.
"""
import argparse
import hashlib
from pathlib import Path


KINDS = ("LINE", "CIRCLE", "ARC", "POINT", "3DFACE", "LWPOLYLINE")


def entity(kind, handle, classes, pairs):
    values = ["  0", kind, "  5", format(handle, "X"), "330", "1F",
              "100", "AcDbEntity", "  8", "0"]
    for cls in classes:
        values.extend(("100", cls))
    for code, value in pairs:
        values.extend((str(code), str(value)))
    return "\n".join(values) + "\n"


def produce_geometry(n):
    # Distinct coordinates, negative coordinates, fractional values, nonzero Z.
    # Distinct high-range entity handles prevent collisions with the base DXF.
    for i in range(n):
        x = (i % 173) * 27.25 - 2018.625
        y = (i // 173) * 39.125 - 400.375
        z = (i % 13) * 0.125
        handle = 0x100000 + i
        kind = KINDS[i % len(KINDS)]
        if kind == "LINE":
            pairs = [(10, x), (20, y), (30, z),
                     (11, x + 1.375), (21, y + 27.5), (31, z + 1)]
            classes = ["AcDbLine"]
        elif kind == "CIRCLE":
            pairs = [(10, x), (20, y), (30, z), (40, 1.5 + (i % 7))]
            classes = ["AcDbCircle"]
        elif kind == "ARC":
            pairs = [(10, x), (20, y), (30, z), (40, 2.75 + (i % 5)),
                     (50, 12.5), (51, 160.25)]
            classes = ["AcDbCircle", "AcDbArc"]
        elif kind == "POINT":
            pairs = [(10, x), (20, y), (30, z)]
            classes = ["AcDbPoint"]
        elif kind == "3DFACE":
            pairs = [(10, x), (20, y), (30, z + 1),
                     (11, x + 4), (21, y), (31, z + 2),
                     (12, x + 4), (22, y + 3), (32, z + 4),
                     (13, x), (23, y + 3), (33, z + 2),
                     (70, i % 4)]
            classes = ["AcDbFace"]
        else:
            pairs = [(90, 4), (70, 1)]
            for j, (dx, dy) in enumerate(((0, 0), (4, 0), (4, 3), (0, 3))):
                pairs.extend(((10, x + dx), (20, y + dy)))
                if i % 4 == 0:
                    pairs.extend(((40, 0.5 + j / 8), (41, 1.0 + j / 8)))
                if i % 3 == 0:
                    pairs.append((42, 0.125 if j == 1 else 0))
            classes = ["AcDbPolyline"]
        yield entity(kind, handle, classes, pairs)


def produce_dense_polyline(n):
    pairs = [(90, n), (70, 1)]
    for i in range(n):
        pairs.extend(((10, float(i) * 0.625 - 10000.0),
                      (20, (i % 11) * 0.125 - 4.5)))
    yield entity("LWPOLYLINE", 0xFABC00, ["AcDbPolyline"], pairs)


def create(source, output, geometry):
    # Keep the base DXF untouched; only mutate the private synthetic copy.
    src = source.read_bytes()
    # Normalize line-endings of the pinned test fixture for deterministic
    # placement. The tests never inspect or upload real user project files.
    text = src.decode("ascii").replace("\r\n", "\n")
    marker = "\n  0\nSECTION\n  2\nENTITIES\n"
    start = text.index(marker)
    end = text.index("\n  0\nENDSEC\n", start)
    with output.open("w", encoding="ascii", newline="\n") as fp:
        fp.write(text[:end+1])
        for chunk in geometry:
            fp.write(chunk)
        fp.write(text[end+1:])
    return output.stat().st_size


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base", type=Path)
    ap.add_argument("output_dir", type=Path)
    args = ap.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for size in (512, 8192):
        target = args.output_dir / f"mixed-{size}.dxf"
        count = create(args.base, target, produce_geometry(size))
        print(f"STRESS_FIXTURE mixed-{size} geometry={size + 1} bytes={count}"
              f" sha256={hashlib.sha256(target.read_bytes()).hexdigest()}", flush=True)
    target = args.output_dir / "dense-polyline-8192.dxf"
    count = create(args.base, target, produce_dense_polyline(8192))
    print(f"STRESS_FIXTURE dense-polyline-8192 geometry=2 bytes={count}"
          f" sha256={hashlib.sha256(target.read_bytes()).hexdigest()}", flush=True)


if __name__ == "__main__":
    main()
