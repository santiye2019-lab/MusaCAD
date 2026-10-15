#!/usr/bin/env python3
"""Generate independent R2000 write/read acceptance cases from pinned LibreDWG DXF.
Only used on CI's host-only writer; never mutates customer drawings or Android.
"""
from pathlib import Path
import sys

def entity(kind, handle, subclasses, pairs, owner="1F"):
    lines = ["  0", kind, "  5", handle, "330", owner,
             "100", "AcDbEntity", "  8", "0"]
    for cls in subclasses:
        lines.extend(("100", cls))
    for group, val in pairs:
        lines.extend((str(group), str(val)))
    return "\n".join(lines) + "\n"

def make_case(src, dst, entities):
    # The pinned fixture uses standard SECTION/ENTITIES DXF layout.
    start = src.index("\n  0\nSECTION\n  2\nENTITIES\n")
    end = src.index("\n  0\nENDSEC\n", start)
    dst.write_text(src[:end+1] + "".join(entities) + src[end+1:])

def main():
    source = Path(sys.argv[1]).read_text()
    target = Path(sys.argv[2])
    target.mkdir(parents=True, exist_ok=True)
    # A closed 4-vertex lightweight polyline with non-zero arcs & widths.
    make_case(source, target / "bulge-width.dxf", [
        entity("LWPOLYLINE", "F100", ["AcDbPolyline"], [
            (90,4), (70,1), (38,0), (39,0),
            (10,0),(20,0),(40,1.25),(41,2.5),(42,0.5),
            (10,25),(20,0),(40,2.5),(41,1.75),(42,0),
            (10,25),(20,15),(40,1.75),(41,1.0),(42,-0.25),
            (10,0),(20,15),(40,1.0),(41,1.25),(42,0),
        ]),
    ])
    # Split arc bulges from variable widths so a failed field is isolated.
    make_case(source, target / "bulge-only.dxf", [
        entity("LWPOLYLINE", "F105", ["AcDbPolyline"], [
            (90,4), (70,1),
            (10,0),(20,0),(42,0.5),
            (10,25),(20,0),(42,0),
            (10,25),(20,15),(42,-0.25),
            (10,0),(20,15),(42,0),
        ]),
    ])
    make_case(source, target / "width-only.dxf", [
        entity("LWPOLYLINE", "F106", ["AcDbPolyline"], [
            (90,4), (70,1),
            (10,0),(20,0),(40,1.25),(41,2.5),
            (10,25),(20,0),(40,2.5),(41,1.75),
            (10,25),(20,15),(40,1.75),(41,1.0),
            (10,0),(20,15),(40,1.0),(41,1.25),
        ]),
    ])
    # Non-coplanar 3D face; edge visibility is an important geometry attribute.
    make_case(source, target / "face3d.dxf", [
        entity("3DFACE", "F110", ["AcDbFace"], [
            (10,0),(20,0),(30,2),
            (11,20),(21,0),(31,3),
            (12,20),(22,15),(32,8),
            (13,0),(23,15),(33,5),
            (70,3),
        ]),
    ])
    # DXF's classic POLYLINE has owned VERTEX_2D records and SEQEND.
    # Explicit handles test whether ownership survives binary encoding.
    make_case(source, target / "classic-polyline.dxf", [
        entity("POLYLINE", "F120", ["AcDb2dPolyline"], [
            (66,1),(10,0),(20,0),(30,0),(70,1),
            (40,1.25),(41,2.25),
        ]),
        entity("VERTEX", "F121", ["AcDbVertex","AcDb2dVertex"], [
            (10,0),(20,0),(30,0),(40,1.25),(41,2),(42,0.5)
        ], owner="F120"),
        entity("VERTEX", "F122", ["AcDbVertex","AcDb2dVertex"], [
            (10,18),(20,0),(30,0),(40,2),(41,1),(42,0)
        ], owner="F120"),
        entity("VERTEX", "F123", ["AcDbVertex","AcDb2dVertex"], [
            (10,18),(20,15),(30,0),(40,1),(41,2),(42,-0.125)
        ], owner="F120"),
        entity("SEQEND", "F124", [], [], owner="F120"),
    ])
    for name in ("bulge-width", "bulge-only", "width-only", "face3d", "classic-polyline"):
        path=target / f"{name}.dxf"
        assert path.stat().st_size > 16
        print("Prepared R2000 acceptance fixture:", name)

if __name__ == "__main__":
    main()
