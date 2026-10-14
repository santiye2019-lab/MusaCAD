# MusaCAD: experimental DXF -> DWG host proof

This proof is intentionally **not part of the Android application**. Normal native
DWG builds keep `LIBREDWG_DISABLE_WRITE=ON`, and no user-facing DWG export,
production APK, Cloudflare Worker, licenses or original drawings are changed.

The pinned GNU LibreDWG supports `dxf_read_file` and `dwg_write_file`, but
its writer is not a lossless modern AutoCAD engine. This pilot writes **R2000 /
AC1015 only** and expressly does not claim to preserve advanced dynamic blocks,
references, proxy objects, annotations or ACIS/B-Rep solids.

For controlled host testing:

```sh
cmake -S app/src/main/cpp -B build-write -DMUSACAD_HOST_DWG_WRITE_PILOT=ON
cmake --build build-write --parallel 2
build-write/musacad_dxf2dwg_pilot input.dxf new-output.dwg
```

Safety invariants:
- Input must be a normal, bounded DXF file (64 MiB maximum).
- Only LINE, CIRCLE, ARC, LWPOLYLINE, POLYLINE_2D, POINT, 3DFACE
  geometry is accepted by the pilot (plus BLOCK/ENDBLK/SEQEND bookkeeping).
  Unsupported drawing entity types refuse the conversion.
- Original input is never opened for writing.
- Destination must **not** already exist, even as a symbolic link.
- An unpublished same-directory temporary DWG is checked for an R2000 header,
  successful fresh LibreDWG read and exact **per-entity-type counts**.
- Primitive coordinates and geometry parameters of LINE, CIRCLE, ARC, POINT
  and 3DFACE are compared after fresh decode, independent of object order.
  Non-finite values and mismatches reject publication. Tolerance is at most
  `max(1e-7, 1e-12 * max(abs(before), abs(after)))` in drawing units.
- Coordinate audit is fail-closed above 250,000 objects; this is a proof
  limit, not an intended production project size.
- Publish is exclusive; on failure the unpublished temporary file is removed.

Counts and partial coordinate checks are still only a pilot: LWPOLYLINE and
POLYLINE_2D vertex data are **not yet** coordinate-verified, nor are colors,
layer styles, block transforms, ownership, annotation or metadata. A CI test
now creates a six-primitive DXF fixture (three LINE, one CIRCLE, one ARC,
one POINT) and verifies that the DWG can be read back with matching counts
and audited primitive coordinates. CI passing would not imply full parity. Hence
**never treat this proof as a production file-conversion guarantee**.

Before an Android-facing DWG Save As feature can be enabled, add robust
primitive-coordinate/attribute verification, check modern version fallbacks,
validate real customer projects with AutoCAD and other viewers, and obtain
physical Android acceptance. Existing 3D editing remains guarded DXF output.

Future ACIS/B-Rep solid editing requires a robust exact geometry kernel with
topological validity and Boolean operation tests. Triangle display tessellation
is not the same as editable exact solids.
