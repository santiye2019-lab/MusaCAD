# MusaCAD — confidential real-DWG read-only acceptance

This is a **separate reader test**, not the R2000/AC1015 writer pilot.
Never upload private drawings to GitHub, Cloudflare, public issues or Actions.
The originals stay unchanged.

## What is measured

The script tests DWG version signatures and SHA-256 before and after reading.
When a native host converter executable is supplied, it also measures DWG
decode and DXF export, collects model entity types/counts, layers, block
definitions, insert references and XREF flags from the resulting ASCII DXF.
It imposes a 180-second process timeout and optional Linux virtual-memory
limit (default 1536 MiB). Temporary converted drawings are deleted.

Run locally outside the repository, with pinned LibreDWG source available:

    cmake -S app/src/main/cpp -B build-host -DCMAKE_BUILD_TYPE=Release
    cmake --build build-host --parallel 2
    python3 tests/private_dwg_read_audit.py \
      --converter build-host/musacad_dwg_convert \
      --report /path/outside/repo/private-audit.json \
      /private/drawings/project1.dwg /private/drawings/project2.dwg

If the native converter is not available, run with --manifest-only and an
explicit --report path instead. The result will say
metadata_only_not_geometry_checked. Such a result must NEVER be described
as successful geometry, layer, block or render verification.

A decoded_dxf_inventory result confirms a conversion and inventory only:
it does not prove lossless geometry, true color, layer visibility, blocks,
XREF resolution, layouts, dimensions, measurements, exact solids or printing.

### Independent primitive geometry cross-check (optional)

For each original DWG, a separately trusted reader may provide a **local-only**
JSON manifest named `<source_sha256>.json` in an external directory, containing
`source_sha256`, `source_kind: "independent_dwg_reader"`, `records: []`,
and `geometry_records`. Each geometry entry includes a source handle,
entity type and numeric DXF group values; use `--independent-manifest-dir`
when running `tests/private_dwg_read_audit.py`. For the simple primitives
below, `groups` is a string-valued DXF group-code map. For **SPLINE**
and **LWPOLYLINE**, repeated values cannot be represented as a mapping:
`groups` must instead be an **ordered array of string pairs**. SPLINE
example: `[["70","8"],["71","2"],["72","6"],["73","3"],["40","0"],...]`.
LWPOLYLINE example: `[["90","2"],["70","1"],["10","0"],["20","0"],["42","1"],["10","4"],["20","0"]]`.
The LWPOLYLINE subset includes declared vertex counts, open/closed flag,
elevation, thickness, constant/vertex widths, signed per-vertex bulges,
optional vertex IDs and OCS normals. A swapped bulge can change the curve
without changing endpoints; the audit checks the original ordered values.
Legacy **POLYLINE** header and **VERTEX** entities are additionally checked
by their original DWG *entity handles*: each reference's `groups` is an
ordinary numeric group-code map. POLYLINE checks header flags, elevation,
widths, mesh settings and normal. VERTEX checks XYZ, widths, signed bulge,
tangent, flags and signed polyface indices (negative indices encode hidden
edges). A missing or changed vertex record must fail the subset comparison.
**This is per-record parity only:** the sequence of VERTEX children under a
POLYLINE parent, the terminating SEQEND and complete polyface topology are
NOT yet independently verified. Do not present this as full mesh validation.
ELLIPSE references instead use the ordinary numeric group mapping, e.g.
`{"10":"1","20":"2","11":"4","21":"0","40":"0.5","41":"0","42":"1.5707963267948966"}`.
Missing start/end parameters are deliberately rejected rather than guessed;
coordinate Z and extrusion defaults are handled by the audited subset.
The independent reader must preserve the covered source group order;
inventing source control points from MusaCAD's own converted DXF is not
independent evidence.

This optional source-bound comparison now covers **LINE** XYZ endpoints,
**POINT** position/normal, **CIRCLE/ARC** center/radius/angle/normal,
**ELLIPSE** WCS center, relative major-axis XYZ, minor/major ratio, required
explicit start/end parameters in **radians**, and plane normal; also
**3DFACE** XYZ corners plus group 70 invisible-edge bits, and the
numeric **SPLINE parameter subset** (degree, flags, ordered knot vector,
control/fit XYZ, rational weights and declared counts), plus **LWPOLYLINE**
ordered vertices, signed bulges, widths and OCS/elevation, plus
per-handle legacy POLYLINE header and VERTEX parameters including signed
polyface face indices. This catches source
curve-definition drift even when entity counts and overall extents match;
it does **not** evaluate curve shape, certify fit-only reconstruction, or prove
real-DWG parity without an independently obtained source manifest. Absent fourth
3DFACE corner means the third corner repeats, while partial fourth-corner
data is rejected. Missing or changed source values produce
`decoded_dxf_inventory_geometry_mismatch`; only aggregate counts appear
in the report, never drawing coordinates, layers or handles. This is a
primitive **subset** proof, not whole-file DWG semantics or a live verification
of private projects. An untrusted baseline derived from MusaCAD's own DXF
output does not count as independent evidence. ACIS/B-Rep, XREF resolution,
dynamic blocks, spline evaluation and real rendering still require separate
acceptance.

For a complete acceptance, export independent baselines from AutoCAD or a
second trusted CAD reader. Compare drawing entity counts, layer/state/color,
blocks/attributes, layout geometry, annotations, coordinates and XREF files.
Physical Android tests must also demonstrate correct initial colors, smooth
zoom and pan, no freezes and acceptable opening times for large drawings.

## GitHub checks

CI runs on upstream redistributable LibreDWG example_2013.dwg and
2000/TS1.dwg fixtures only. Actual customer or municipal files are never
included, uploaded or stored as artifacts.

The user's private DWGs have been verified only for header version, size
and unchanged SHA-256 in the working environment. Native geometry read for
them remains NOT VERIFIED while a runnable host decoder is unavailable.
Production APK, main and DWG Save As stay untouched.
