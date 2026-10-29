# Independent 2D scene evaluator provenance (2026-10-28)

- **Scope:** `core/firstparty/scene2d/` of the original MusaCAD repository, **not** MusaCAD2.
- **Origin:** Independently authored C11 implementation from basic affine matrix algebra, binary search, graph traversal and generic CAD BYLAYER/BYBLOCK/Layer 0 behavior. No source code was imported from any existing MusaCAD, LibreDWG, third-party CAD project or other GPL/AGPL/LGPL library.
- **License:** MIT, see SPDX headers. Only standard C11 runtime and math library required.
- **Build boundary:** Separate CMake target and standalone tests. **Not linked to** `musacad_core` or the Android production DWG JNI. The existing core contains GPL-marked files and requires separate provenance/license review.
- **Capabilities:** Validates nested INSERT parent graphs, duplicate and dangling handles, layers, layer-0 inheritance, BYLAYER/BYBLOCK colors, inherited visibility, and composed 2D affine transforms. O((nodes+layers) log(nodes+layers)) iterative resolution; output rollback on failure.
- **Limitations:** Accepts already decoded nodes/layers, **not** AC1032 bytes. No viewport freeze, plot-style, ACI palette, XREF, dynamic block, font, or genuine DWG file support claimed. Production use requires integration and real DWG evidence.
- **Local verification:** `cmake -S core/firstparty/scene2d -B <build> -DBUILD_TESTING=ON && cmake --build <build> && ctest --test-dir <build> --output-on-failure`.

## Viewport culling addition (2026-10-28)

- **New files:** `include/musa_scene2d_cull.h`, `src/musa_scene2d_cull.c`, and `tests/scene2d_cull_test.c`.
- **Source origin:** Independently authored from elementary affine transformation of four axis-aligned rectangle corners and viewport intersection inequalities. No legacy MusaCAD/LibreDWG source, copyleft library, or third-party implementation imported or linked.
- **Runtime:** Standard C11 plus the system math library; O(n) candidate collection without heap allocation. Output and count remain unchanged on malformed geometry, overflow, aliasing, or insufficient capacity.
- **Coverage:** Rotation, reflection/shear-compatible corner transforms, inherited layer visibility, hidden geometry, viewport edge contact, malformed bounds, nonfinite arithmetic, and rollback.
- **Limits:** Conservative screen-space candidate selection over *already resolved* scene geometry; not AC1032 decoding, spatial indexing, exact clipping, or evidence of complete DWG rendering. New CI must pass before describing the addition as verified.

## Viewport-specific layer freeze adapter (2026-10-28)

- **New files:** `include/musa_scene2d_viewport.h`, `src/musa_scene2d_viewport.c`, and `tests/scene2d_viewport_test.c`.
- **Origin:** Independently written C11 adapter based on combining boolean layer visibility and invoking the existing independent scene graph evaluator. No source imported from legacy MusaCAD, LibreDWG, or other third-party CAD code.
- **Behavior:** Per-viewport layer eligibility is combined with global layer visibility without mutating the source layer table. Frozen INSERT ancestors hide descendants, even if descendants have their own nonfrozen layers; effective layer-0 inheritance, BYLAYER/BYBLOCK colors and world transforms remain intact.
- **Failure policy:** Invalid masks or global visibility flags, buffer overlap, malformed ownership, and insufficient capacity leave output untouched. The implementation allocates one temporary layer array and retains the existing iterative O((nodes+layers) log(nodes+layers)) resolver.
- **Limits:** This API receives predecoded layer flags and scene nodes. It does not decode AC1032 viewport records, derive VP-FROZEN lists from DWG, handle layout-specific display states, or enable production DWG loading. Validate CI and real drawing fixtures before treating it as production-ready.

## Cached 2D world bounds (2026-10-29)

- **New first-party files:** `include/musa_scene2d_cache.h`, `src/musa_scene2d_cache.c`, and `tests/scene2d_cache_test.c`. Source independently authored using elementary four-corner affine AABB transformation and rectangle intersection; no LibreDWG, legacy GPL code, or external CAD implementation imported.
- **Purpose:** Compute conservative world-space bounds once after scene resolution, then reuse them for repeated pan/zoom viewport queries without repeating four-corner matrix transformations. Hidden nodes and INSERTs are not drawable; cached results must be rebuilt when geometry, transforms, layer visibility, or viewport freeze state changes.
- **Safety:** Reject malformed handles, visibility values, nonfinite transforms, invalid local boxes, overflowing affine results, output/input aliasing and insufficient capacity. Both APIs validate before writing; failed calls leave caller outputs unchanged.
- **Tests:** Compare cached queries against the existing independent uncached collector (including viewport-edge contact and frozen parent INSERT), check rollback on malformed data and aliasing, and query a synthetic 20,000-object scene. No performance benchmark or production DWG file evidence is claimed.
- **Boundary:** Remains a standalone MIT C11 target, not linked to the GPL-marked historical native core or production Android JNI. Does not parse AC1032 bytes, render text, decode ACI colors, or prove full DWG interoperability.

## Analytic primitive bounds (2026-10-29)

- **New files:** `include/musa_scene2d_geometry.h`, `src/musa_scene2d_geometry.c`, `tests/scene2d_geometry_test.c`.
- **Source origin:** Independently authored using elementary line extrema, circle radii and trigonometric arc extrema at quadrant angles. No code from existing CAD readers, LibreDWG, or GPL/AGPL/LGPL libraries was copied, adapted or linked.
- **License and boundary:** MIT SPDX headers, standard C11 `math.h` and `float.h` only. Part of the isolated `musacad_independent_scene2d` target, not the existing GPL-marked native CAD core or Android DWG JNI.
- **Coverage:** LINE, CIRCLE and counterclockwise ARC local AABBs; wraparound, full-circle sweeps, zero-sweep arcs, invalid/nonfinite inputs, overflow, output rollback and 5000 sampled arc sweeps.
- **Limitations:** Requires already decoded primitive coordinates; it does not parse DWG, resolve OCS extrusion or elevation, interpret text/fonts or bulged polylines, render geometry, or demonstrate successful real-file opening. The input coordinates must be in the same local coordinate system as the scene resolver.

## Full-ellipse analytic bounds extension (2026-10-29)

- **Scope:** Added `musa_scene2d_ellipse_bounds` to the same independent MIT C11 geometry module. The implementation follows directly from the norms of the two Cartesian sinusoidal coefficients of an ellipse with major-axis vector `(ax, ay)` and perpendicular minor axis `ratio*(-ay, ax)`. No third-party CAD code, GPL/LGPL/AGPL sources or legacy MusaCAD implementation was imported.
- **Validation:** Positive finite major-axis length, minor/major ratio in `(0,1]`, finite center and extents; rejects invalid or overflowing input without altering the output. Tests sample 4096 points on a rotated ellipse, check exact axis extrema, invalid inputs, rollback, and composition with nested-block world bounds and viewport culling.
- **Limits:** Only **full planar ellipses** after coordinate decoding; not partial ELLIPSE arcs, OCS/extrusion, real DWG object parsing, or Android rendering. These limitations must not be represented as full AC1032 support. CI must be checked for the new commits.

## Decoded 2D polyline bulge bounds (2026-10-29)

- **New files:** `include/musa_scene2d_polyline.h`, `src/musa_scene2d_polyline.c`, and `tests/scene2d_polyline_test.c`. Source independently authored from elementary circle geometry: a bulge is the tangent of one quarter of the signed arc angle. No third-party CAD source or GPL/AGPL/LGPL implementation was imported, adapted, or linked.
- **Build:** The independent MIT C11 target compiles the polyline module and runs its dedicated regression test on supported CI platforms. The only dependency is the same target's standalone primitive bounds helper and standard C math library.
- **Coverage:** Open and closed polylines, line segments, signed semicircles, major circular arcs, near-straight bulges, degenerate segments, malformed or nonfinite inputs, numeric overflow, overlapping output storage and rollback.
- **Limits:** The API consumes **already decoded planar XY vertices and bulges**. It does not parse DWG LWPOLYLINE/POLYLINE records, decode extrusion/OCS or thickness, perform stroke rendering, or establish actual AC1032 interoperability. No connection to the legacy GPL-marked native core or Android JNI is made.
