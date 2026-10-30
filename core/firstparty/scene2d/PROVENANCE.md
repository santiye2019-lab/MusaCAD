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

## Partial ELLIPSE arc analytic bounds (2026-10-29)

- **Files:** Added `musa_scene2d_ellipse_arc_bounds` to the independently authored MIT C11 geometry header and source, with `tests/scene2d_ellipse_arc_test.c` and a dedicated CTest target.
- **Source origin:** Elementary parametric ellipse differentiation: each Cartesian component is `A*cos(t)+B*sin(t)`, whose stationary parameters are `atan2(B,A)` and that angle plus pi. No legacy MusaCAD, LibreDWG, third-party CAD, GPL/AGPL/LGPL source or codec was copied, adapted or linked.
- **Behavior:** Conservative axis-aligned bounds for decoded planar ELLIPSE segments, including rotated major axes, wrapped CCW parameter intervals, zero-sweep points, and full revolutions. Rejects invalid, nonfinite or overflowing inputs without modifying the caller's output.
- **Tests:** Analytic quarter-ellipse and wrapped extrema, full and zero sweep, 1000 sampled rotated sweeps, invalid-input rollback, and nested INSERT/viewport cache composition. GitHub CI outcomes must be checked separately.
- **Limitations:** Requires already decoded ellipse geometry and planar coordinates; it does not parse AC1032 records, resolve OCS/extrusion, draw glyphs or integrate with production Android JNI. This does not establish real-file DWG reading or rendering. The module remains separate from the historical GPL-marked native core.

## Independent UTF-8 and decoded text ink bounds (2026-10-29)

- **Files:** `include/musa_scene2d_text.h`, `src/musa_scene2d_text.c`, `tests/scene2d_text_test.c`, and the standalone CMake/CTest registration.
- **Origin:** Independently authored strict UTF-8 Unicode-scalar validation and elementary pen-advance plus four-corner affine bounds accumulation. No GPL/AGPL/LGPL source, legacy CAD code, font shaping implementation, DWG decoder, or third-party library was imported, adapted or linked. MIT SPDX, C11 standard library and math only.
- **Coverage:** Rejects overlong encodings, surrogates, out-of-range scalars, embedded NUL, truncation and malformed continuation bytes; text glyph bounds handle per-glyph advances, blank glyphs, rotated/sheared/scaled text transforms, numeric overflow, aliasing and rollback. Includes a 20,000-glyph synthetic test.
- **Critical limitation:** The input is already-decoded UTF-8 and **externally shaped font glyph ink metrics**. Glyph count can differ from codepoint count. The code does **not** decode AC1032 TEXT/MTEXT records, process MTEXT controls, implement SHX/TTF fonts or shaping, handle alignment, prove glyph accuracy, render text, or enable Android DWG import. Real-file and font-license/provenance tests are still required. Keep the existing production JNI fail-closed gate intact.
- **Validation:** The new CTest target is `independent_scene2d_utf8_text_ink_bounds`. Successful CI must be independently confirmed; adding tests is not evidence that they passed.

## Predecoded ACI / true-color adapter (2026-10-29)

- **Files:** `include/musa_scene2d_color.h`, `src/musa_scene2d_color.c`, and `tests/scene2d_color_test.c`. Independently authored from basic indexed-color and RGB arithmetic; no code from legacy MusaCAD, LibreDWG, GPL/AGPL/LGPL or other CAD readers was copied, adapted, or linked.
- **Behavior:** Converts already decoded ACI 0 (BYBLOCK), 256 (BYLAYER), canonical 1–6, foreground-dependent 7, caller-provided 256-entry ACI palette colors, and 24-bit true-color overrides into the existing scene resolver's modes and opaque RGBA values. Explicit color and nested INSERT/layer-0 inheritance are exercised together.
- **Safety:** Rejects unsupported ACI 8–255 without a verified caller-supplied palette, invalid color flags/values, incomplete palettes and overlapping buffers; failed calls leave output untouched. No approximate fallback palette is invented.
- **Boundary:** Standard C11 in the isolated MIT target. No DWG byte decoding, palette extraction, alpha/transparency, color-book or plot-style handling, production Android JNI integration, or proof of real AC1032 color fidelity. The application must supply a vetted palette and decoded color records; the existing GPL-marked native core remains unlinked. New GitHub CI results must be checked independently.

## Strict UTF-16LE text decoding boundary (2026-10-29)

- **New independent source:** `include/musa_scene2d_utf16.h`, `src/musa_scene2d_utf16.c`, and `tests/scene2d_utf16_test.c`. Written from Unicode scalar and UTF-8 encoding rules without importing, translating, adapting or linking any existing CAD parser, GPL/AGPL/LGPL code or third-party library.
- **Purpose:** Decode bounded, already located UTF-16LE text and layer-name payloads into UTF-8 while preserving supplementary-plane characters and combining marks. Two-pass conversion supports required-size queries, requires a NUL terminator in destination capacity, rejects embedded NULs, odd lengths and invalid surrogate pairs, and leaves output untouched on failure.
- **Tests:** ASCII, Latin, euro, CJK, supplementary-plane and combining characters; invalid surrogates, truncation, output capacity, pointer aliasing, empty payloads, and 20,000-character conversion checked against the independent strict UTF-8 validator. Cross-platform C11 CI must be inspected before claiming success.
- **Limits:** This is **not** an AC1032 section/record reader or a DWG string-location decoder. It does not interpret MTEXT formatting, legacy code pages, font shaping, or text glyphs. It is not connected to the Android production JNI. Existing 3D source and GPL-marked legacy code are untouched.

## Record-bounded UTF-16LE terminated fields (2026-10-29)

- **API:** `musa_scene2d_utf16le_cstr_to_utf8` scans a caller-supplied bounded byte region for an aligned UTF-16LE NUL code unit, validates its Unicode scalar payload and returns the number of record bytes consumed, including the terminator. Trailing bytes are left uninterpreted; odd record bounds are permitted when a complete string precedes them.
- **Origin:** Independently authored from elementary byte scanning and the existing first-party strict UTF-16LE adapter. No third-party DWG reader, legacy GPL/AGPL/LGPL implementation, or unverified code imported or linked.
- **Safety:** Missing terminator, unpaired surrogate, malformed pointers, overlap, or insufficient destination capacity fail without changing the output or either count. The original bounded non-terminated converter is unchanged.
- **Tests:** Dedicated assertions in `scene2d_utf16_test.c` exercise supplementary Unicode, trailing record bytes, empty strings, size queries, truncated records, invalid surrogates, aliasing and rollback. CI status must be verified independently.
- **Limitations:** This is a general bounded UTF-16LE field adapter, **not** a DWG object/section reader. It cannot locate AC1032 fields, interpret compressed/encoded string sections, decode font information, or establish real-file interoperability. Existing Android DWG JNI remains fail-closed and 3D code is untouched.


## Caller-owned spatial index for large 2D drawings (2026-10-29)

- Added `musa_scene2d_index_build` and `musa_scene2d_index_query` to the original MusaCAD first-party 2D scene module. Source is independently authored from elementary binary-tree bounding-box unions and standard C11; no external CAD source, GPL/AGPL/LGPL implementation, or legacy core linkage.
- A complete binary tree of conservative world-space AABBs prunes whole subtrees outside the viewport. It uses caller-provided storage, no recursion in building, and bounded logarithmic recursion during queries. Query results retain source draw order. Building is O(n), query is O(n) worst case; clustered input can significantly reduce visited nodes. No benchmark speedup is asserted without measurements.
- The index consumes only previously resolved and cached 2D entity bounds. Rebuild on any geometry, layer visibility, transform, or viewport-freeze change. Its storage must not be mutated after successful build. Failed builds and insufficient query capacity preserve caller outputs.
- Standalone CTest compares 500 viewports against the existing linear collector across 20,000 synthetic entities and checks edge inclusion, hidden nodes, invalid bounds, aliases and atomic failure. This does not validate real AC1032 DWG reading, rendering or Android performance.

## Planar block INSERT affine adapter (2026-10-29)

- **Files:** `include/musa_scene2d_insert.h`, `src/musa_scene2d_insert.c`, `tests/scene2d_insert_test.c`. First-party MIT/C11 implementation derived from elementary 2D affine matrix algebra; no code from GPL/AGPL/LGPL, LibreDWG, legacy CAD implementations, or unverified third-party sources was copied, adapted or linked.
- **Behavior:** Given already decoded insertion point, block base point, planar X/Y scales and rotation (radians), computes the scene resolver's six-coefficient affine transform. Preserves block base-point alignment, negative mirrored scales, zero-scale degenerate geometry, and nesting through the existing scene graph. Tests also check inherited layer-0/BYBLOCK colors and 20,000 deterministic base-point placements.
- **Safety:** Rejects nonfinite inputs, nonfinite output coefficients, null pointers, and input/output aliasing. Failed conversions preserve output. The source is included only in the independent scene2d static target and a dedicated CTest; this does not change Android JNI or existing 3D sources.
- **Boundary:** This adapter does not decode AC1032 DWG object records, recover INSERT base points, perform OCS/extrusion-to-WCS conversion, resolve arbitrary 3D blocks, or prove that any real DWG opens. Production import must remain fail-closed pending end-to-end real-file tests. CI outcomes must be checked independently.

## Decoded planar INSERT arrays (2026-10-29)

- **Source:** `musa_scene2d_array.h/.c` and `scene2d_array_test.c` are independently authored from planar affine algebra, MIT SPDX and standard C11. No GPL/AGPL/LGPL or uncertain-origin code imported or linked.
- **Behavior:** A selected row/column of a predecoded rectangular array receives the original block-base transform plus a block-local spacing offset transformed by scale and rotation. Invalid dimensions, nonfinite arithmetic, aliases and out-of-range indices fail without modifying output.
- **Limits:** Array spacing is defined here in local block axes; this contract must be checked against actual DWG fixtures. No AC1032 MINSERT decoding, OCS/extrusion, Android JNI or real-file rendering. Existing 3D source remains unchanged.

## Batch expansion of decoded planar INSERT arrays (2026-10-29)

- `musa_scene2d_array_fill` emits row-major affine matrices for already-decoded arrays, with a preflight pass before any output writes, checked size multiplication and no allocation. The 20,000-instance test checks coordinates, negative scaling, count/capacity rejection and rollback.
- All source is independently authored MIT C11, with no legacy CAD or GPL/AGPL/LGPL code linked. This is not a DWG array-record parser or a verified renderer. Caller inputs must stay immutable during the two passes; large arrays still require caller-owned output memory.

## Decoded layer color and visibility adapter (2026-10-29)

- **Files:** `include/musa_scene2d_layer.h`, `src/musa_scene2d_layer.c`, `tests/scene2d_layer_test.c` and `tests/scene2d_layer_integration_test.c`. Independently authored MIT C11 logic; standard integer sign/flag tests and the existing first-party color adapter only. No GPL/AGPL/LGPL, LibreDWG, legacy native core or uncertain-origin source was copied, adapted or linked.
- **Behavior:** Maps a previously decoded signed ACI (-255..-1 means off), frozen flag, selected-viewport frozen flag and optional truecolor to a scene layer. Truecolor never erases the layer-off sign; an indexed ACI 8..255 requires a separately verified RGB palette. ACI 7 uses the supplied foreground or vetted palette. The batch converter preflights all records before writing, with O(n) time and O(1) scratch memory.
- **Tests:** Single-record and 20,000-record mapping, off/frozen/viewport-frozen combinations, truecolor and palette colors, invalid values, output aliasing, atomic batch rollback; a scene-graph regression checks layer-0 and BYBLOCK inheritance and rejects non-INSERT ownership.
- **Limits:** Inputs are already decoded. This is not AC1032 DWG layer-table decoding, per-viewport overrides, entity invisibility flags, a vetted ACI palette, Android rendering, or real-file evidence. Preserve fail-closed import until independently licensed real DWG fixtures pass. Existing 3D sources and MusaCAD2 remain unchanged.

## Decoded planar TEXT placement (2026-10-29)

- **Files:** `include/musa_scene2d_text_placement.h`, `src/musa_scene2d_text_placement.c`, `tests/scene2d_text_placement_test.c`; registered in the standalone C11 scene2d CMake/CTest target.
- **Origin/license:** Independently authored from elementary 2D affine rotation, scaling, shear and anchor translation. MIT SPDX. Only C11 math and standard headers. No legacy MusaCAD, LibreDWG, GPL/AGPL/LGPL or unknown-origin source copied, adapted or linked.
- **Inputs:** Already decoded insertion coordinates, height, width factor, oblique/rotation angles and an alignment anchor supplied in *normalized, already shaped glyph coordinates*. A negative width factor supports planar horizontal mirroring. The anchor is measured by the caller, not guessed from DWG justification codes.
- **Safety/tests:** Rejects nonfinite and degenerate inputs, overflowing matrix coefficients, aliased output and invalid pointers without changing the destination. GCC/Clang local standalone tests exercised anchors, rotation, oblique shear, reflection, 20,000 parameter combinations and glyph-box corner bounds; GCC AddressSanitizer/UndefinedBehaviorSanitizer passed locally. GitHub CI outcomes require independent confirmation.
- **Limitations:** No actual AC1032 TEXT/MTEXT record reading, OCS/extrusion, font lookup, shaping, multiline layout, style interpretation, exact justification or Android JNI connection. Does not establish real DWG file compatibility. Existing 3D and legacy core remain untouched.

## Bounded viewport-result pagination (2026-10-30)

- **Files:** Extended the existing independent `musa_scene2d_index.[ch]` and its CTest regression suite with `musa_scene2d_index_query_page`. The function reports total matches and writes only a caller-sized page in source order, including count-only queries, without per-query heap allocation. This avoids requiring a full-size result array for dense drawings.
- **Origin:** Independently authored from elementary binary-tree traversal and checked integer bounds, MIT SPDX; no code from LibreDWG, legacy MusaCAD CAD readers, GPL/AGPL/LGPL or uncertain-origin implementations was imported, adapted or linked.
- **Tests:** Page offsets, zero capacity, out-of-range offsets, invalid viewports, aliased output buffers, rollback, and ordered equivalence against a full query over 20,000 synthetic objects. GitHub CI must be checked for the final commit before marking tests passed.
- **Performance boundary:** One page needs a count pass and a pruned traversal; repeatedly requesting many pages revisits the tree. This is a memory-bounded API, **not** a measured speedup. It works on already prepared 2D bounds and does not read AC1032 pages, extract entities, render Android geometry, or prove real DWG interoperability. Production JNI remains fail-closed. Existing 3D code, `MusaCAD2/`, and `main` were not changed.

## Spatial-index finite-bound and output-pointer hardening (2026-10-30)

- **Scope:** Existing independently authored MIT C11 `musa_scene2d_index.c` and its standalone regression tests only; no production DWG reader, Android JNI, 3D code, `main`, or `MusaCAD2/` changes.
- **Fixes:** A nonempty query output capacity now requires a non-null indices pointer, including zero-hit viewports, preventing a null write when hits exist. Visible prepared AABBs must have finite, ordered coordinates; nonfinite visible bounds can no longer enter the spatial index. Hidden entries remain excluded even when their geometry is unavailable.
- **Safety tests:** Null query output with and without viewport hits, output-count rollback, infinity in minimum/maximum bounds, index-storage rollback, and hidden malformed bounds.
- **Origin:** Independently reasoned from C pointer validity and IEEE-754 finite-coordinate requirements; no external CAD source, GPL/AGPL/LGPL implementation, or copied algorithm.
- **Limit:** These are scene-level robustness fixes for predecoded geometry, not AC1032 decoding or evidence that real DWG drawings render. Confirm GitHub CI for the final commit before calling tests successful.

## Finite world bounds at IEEE-754 coordinate limits (2026-10-30)

- **Change:** Independently authored C11 scene-cache and direct-culling bounds now widen toward the largest finite double instead of infinity. At exactly +/-DBL_MAX the conservative bound remains finite, allowing a successfully prepared drawing to enter the spatial index. The linear prepared-bounds collector rejects externally supplied nonfinite visible boxes consistently with index construction.
- **Regression:** Two synthetic point entities at +DBL_MAX and -DBL_MAX are checked through direct culling, cached queries and spatial indexing. A corrupt infinity bound is rejected without changing output counts or indices. This is a numerical edge-case fix, not a measured large-drawing performance gain.
- **Provenance:** Standard C11/IEEE-754 arithmetic and independently authored tests only. No legacy CAD reader, LibreDWG, GPL/AGPL/LGPL code, or unknown-origin implementation was copied, adapted or linked. Existing 3D code, `MusaCAD2/`, `main` and production JNI were not modified.
- **Boundary:** AC1032 compressed pages, actual entity decoding, original colors/layers, block semantics and Android rendering remain unverified against real DWG fixtures. CI must be checked on the final commit; synthetic geometry tests are not proof of real DWG import.

## 2026-10-30 — resumable source-index viewport pagination

- Added `musa_scene2d_index_query_from` to the independent 2D AABB tree. A caller starts at source index zero and resumes from the last returned index plus one. Unlike hit-offset pagination, earlier source ranges are pruned without recounting all previous hits on every page; no heap allocation or changes to the existing index ABI are required. Source drawing order and edge-touch visibility are preserved.
- Added a standalone CTest covering invalid pointers, output rollback, hidden entries, viewport boundaries, 20,000 synthetic entities, 80 viewports, and full-query versus successive-page equivalence. The API accepts **already prepared** scene bounds; it is not an AC1032 decoder, Android integration, real-DWG performance benchmark or claim of end-to-end rendering.
- **Origin:** Original MIT C11 implementation from ordinary binary-tree traversal and range exclusion. No GPL/AGPL/LGPL, LibreDWG, uncertain-provenance CAD implementation, legacy MusaCAD core or SDK source was copied, adapted or linked. Production JNI, existing 3D code, `MusaCAD2/` and `main` were not changed. Exact-commit CI results must be checked separately.

## 2026-10-30 — independent arbitrary INSERT transform batch and paging

- **Scope:** Added `musa_scene2d_insert_affine_batch` and `musa_scene2d_insert_affine_page` to the existing standalone C11 2D scene module. These adapt arrays of **already decoded planar INSERT placement fields** into affine transforms. Unlike rectangular MINSERT array generation, each input can have independent translation, scale, base point and rotation. No existing 3D code, Android JNI, `MusaCAD2/` or `main` is modified.
- **Safety/performance:** Whole-batch preflight preserves all output transforms on a late malformed placement; page preflight validates only the requested range, O(page size), for large drawings. Both paths use caller-provided memory, reject overlapping buffers and invalid finite transforms, and never allocate. Zero-capacity page queries return a total without parsing unrelated entries.
- **Synthetic regression:** 20,000 independently positioned INSERTs, late NaN/infinite transforms, undersized capacity, source/output aliases, 16-entry pages and empty pages. The preexisting CTest target `independent_scene2d_nested_insert_placement` exercises both APIs; CI must be checked for the exact commit.
- **Origin and limit:** Original MIT C11 arithmetic and elementary bounds checks, without copying/adapting/linking GPL/AGPL/LGPL, LibreDWG, historical CAD implementation or uncertain-origin code. No actual AC1032 entity decoding, compressed-page recovery, DWG text/owner/layer extraction or Android real-file rendering is established by these synthetic tests.

## 2026-10-30 — finite large-coordinate INSERT composition

- **Fix:** The independent planar INSERT placement and nested scene-graph affine composition now use standard C11 `fma` multiply-add operations to avoid rejecting finite results when a large intermediate product would overflow before cancellation. This targets legitimate large-coordinate CAD drawings; genuinely nonfinite final matrices are still rejected, preserving transactional output behavior.
- **Regression:** Synthetic tests cover finite `1e308` insertion/base/scale combinations, nested INSERT translation and matrix-coefficient cancellation, genuinely overflowing results, and unchanged outputs on failure. These are numerical edge cases, **not** real-file AC1032 or Android rendering tests. The exact final-commit CI result must be checked separately.
- **Origin:** Original MIT C11 code based solely on standard IEEE-754 fused arithmetic and independently authored regression cases. No third-party CAD implementation, LibreDWG, GPL/AGPL/LGPL or uncertain-provenance code was copied, adapted, linked or consulted as an implementation template.
- **Limit:** Fused operations reduce false overflow and rounding, but cannot recover information already lost in a double input or guarantee cancellation across all three affine terms. Actual AC1032 compressed pages, entity records, layer/text/block parsing, real-file fixtures and production JNI import remain unverified. Existing 3D code, `MusaCAD2/` and `main` are unchanged.

## 2026-10-30 — large-coordinate text and viewport consistency

- **Change:** Standard C11 `fma` is now used when subtracting a decoded text alignment anchor from its insertion point and when transforming entity corners for both cached and direct viewport culling. This reduces cancellation-related rounding discrepancies in very large 2D drawings and keeps the two culling paths numerically consistent.
- **Independent origin:** Elementary affine algebra and IEEE-754 fused arithmetic, authored within the MIT-only `core/firstparty/scene2d` target. No GPL/AGPL/LGPL, LibreDWG, historical CAD reader or unknown-origin source was copied, adapted or linked.
- **Regression:** A synthetic text anchor at 1e16 and a synthetic point at -1e16 whose transformed location is sub-unit compare fused reference arithmetic and direct, cached and indexed viewport results. Existing 20,000-item tests remain in their CTest targets. CI outcomes must be verified for the final commit.
- **Limits:** These changes operate on already decoded text placements and scene transforms. They do not locate or decode real AC1032 pages, layer tables, TEXT/MTEXT records or fonts, and do not establish actual Android DWG rendering. Fused arithmetic cannot restore precision lost in input doubles or guarantee a finite result for every possible cancellation. Existing 3D work, production JNI, `MusaCAD2/` and `main` remain untouched.

## 2026-10-30 — large-coordinate MINSERT array translation

- **Change:** The independently authored planar rectangular INSERT-array adapter now combines each transformed row/column spacing with the base translation using C11 `fma`. This avoids false overflow when a large intermediate scaled offset cancels against a large but finite insertion translation. Finite final coordinates are preserved; nonfinite final results are rejected.
- **Regression:** `scene2d_array_test.c` checks a 2-column synthetic array where an intermediate `2 * 1e308` overflows in ordinary double multiplication but the final translated X coordinate is representable. Single-instance, whole-array and page-based paths must agree. CI status is checked separately and should not be inferred from this documentation.
- **Origin:** Standard affine algebra and C11 fused multiply-add, written independently for the MIT first-party module. No legacy MusaCAD, LibreDWG, GPL/AGPL/LGPL, or uncertain-origin source was copied, adapted or linked. The existing 3D code and production JNI are not changed.
- **Boundary:** This consumes predecoded planar array dimensions and placements. It is not an AC1032 MINSERT decoder, does not recover compressed DWG pages, and does not establish complete original layer/color/text fidelity or Android real-DWG opening.
