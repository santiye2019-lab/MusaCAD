# MusaCAD – 2D/3D CAD engine versus AutoCAD desktop / mobile gap audit
Date: 2026-10-12 | Scope: repository `santiye2019-lab/MusaCAD` on `main`; **static code audit, not a device acceptance certificate**.

## Baseline and interpretation
- Current Android quality gate and isolated Linux Native CAD Debug/Release regressions succeeded on PR #241, but these don't exercise all DWG entity types and Android real-device frame timings.
- Treat AutoCAD **desktop** as advanced-editing reference. AutoCAD **mobile** has a different, smaller tool set. Autodesk's 2025 mobile support article explicitly says native 3D modeling is not supported; MusaCAD's real 3D orbit/vertex tools can differentiate it, but are not equivalent to desktop ACIS editing.
- Status legend: **C = confirmed limitation from source**, **R = technical risk requiring fixture/phone confirmation**, **P = partial implementation**. Never claim an end-user symptom fixed from a passing code contract test.

## P0 – data integrity and smooth opening
| ID | Subsystem | Status and evidence | Required completion / acceptance |
|---|---|---|---|
| C-01 | 3D-only DWG/DXF | **C**: `DxfParser.java` does not emit 2D entities for `3DSOLID/BODY/REGION`; `renderStreaming()` returns null for empty 2D entity list; `MainActivity.java` ~4281–83 rejects null 2D content, even if `Dxf3dMesh` supports the solid. | Accept genuine 3D-only DXF/DWG with 3D route, verify ACIS data and no fabricated geometry; issue #242. |
| C-02 | Edited DWG write | **C**: `MainActivity.requestEditedDxfSave()` names output *_duzenlendi.dxf, `DxfWriter` writes only DXF. In CMake `LIBREDWG_DISABLE_WRITE ON`. Original DWG is preserved but cannot be saved as edited DWG. | Complete DXF regression, then investigate standards-compatible DWG writer with safe roundtrip for all test entities and a separate output; issue #244. |
| C-03 | 3D source editing | **C**: `Mesh3dActivity` permits single-vertex X/Y/Z adjustment only for non-ACIS editable meshes. `Dxf3dEditor` intentionally rejects edited coordinates from nested transformed blocks. No ACIS B-Rep edit/write. | Stage 3D vertex transform tools, solid modeler later; keep undeclared objects read-only. |
| R-04 | DWG first paint / edit latency | **R**: `MainActivity` copy → native load → native scene → DXF export → authoritative DXF parse. Requires full memory/cpu benchmark. | Timestamp stage latencies and frame p50/p95 in debug/test builds with ≥3 sizes and repeatable device profiling; issue #243. |
| R-05 | Native color parity | **R + field symptom**: first-pass native scene and final DXF use different paths; user observed blue first frame corrected after zoom. Existing color method unit tests do not prove pixel parity across nested blocks and full projects. | Compare object ID/color between initial/final stage; test TrueColor, ByLayer, ByBlock, negative/frozen layers and theme; issue #243. |
| R-06 | 3D navigation performance | **R**: `Mesh3dView.onDraw` projects every vertex, sorts all triangle faces via `Arrays.sort(faceOrder)` every frame and draws all edges. `nearestVertex` is linear. | Instrument p50/p95 FPS/CPU; add spatial acceleration, triangle clipping/culling, progressive meshes / GPU renderer when justified. |
| C-07 | Large-file boundaries | **C**: `DxfStream.MAX_BYTES` = 512 MiB; `Dxf3dMesh` caps vertices 2m / triangles 4m / edges 4m; `MainActivity` blocks opening another file while one project is preparing; at most four tabs. | Bounded memory/safe caching and user-visible truthful limits; test no ANR/OOM, device storage behavior and switching between four heavy projects. |
| R-08 | Write roundtrip coverage | **R**: no dedicated `DxfWriterTest.java` or `Dxf3dEditorTest.java` discovered in `tests/`. In-place DXF content modifications/partial 3D tag replacements need end-to-end validation. | Open/edit/save/reopen/compare geometry, metadata, color, layer, OLE and unicode. Verify original unchanged; issue #244. |
| C-09 | Raster image references | **P**: `DxfParser` converts DXF `IMAGE` entity to a boundary polyline. `CadImageOverlayStore` preserves user images as sidecar MusaCAD metadata, not embedded in DXF. | Resolve external references or show missing path, preserve image transformations, avoid claiming image embedded on Save. |

## P1 – 2D completeness, CAD compatibility
| ID | Feature | Findings / next validation |
|---|---|---|
| C-10 | External XREF DWG/image, bind/reload, nested references | `DxfBlocks` skips certain XREF BLOCK flags, and no full external DWG resolution manager is in this app path. Implement explicit missing reference list + attach/reload and nested path handling; do not pretend XREF geometry belongs to main DWG. |
| C-11 | Dynamic block actions / grips / visibility state | Ordinary BLOCK/INSERT and MINSERT parsing exist. No full AutoCAD dynamic block parameter/action editor found; build inspect-only preservation, then safe editing. |
| P-12 | Paper space, layout and viewports | `DxfParser` parses layout/viewports, but full viewport-accurate paper/model transformations, view clipping, UCS and layer VP freeze need fidelity corpus. |
| P-13 | Dimensions + annotation | Linear/aligned/angular/radius/diameter UI tools exist; read-time DIMENSION uses simplified lines or block picture, not a complete associative dimension/annotation engine. Verify DIMSTYLE, MTEXT, annotative scales, field text and associative updates. |
| P-14 | HATCH and advanced curve types | HATCH, SPLINE, LWPOLYLINE, MLINE, LEADER, OLE and XLINE have parsers but not necessarily full CAD fidelity. Add audit corpus for gradients, islands, custom hatch patterns, spline knot/weight integrity. |
| P-15 | DWG objects and proxy objects | Unsupported objects counted/skipped but no complete metadata/collision safe preservation proof on write. Enumerate unsupported type/handle/layer and warn before modifying. |
| P-16 | 2D command fidelity | `CadCommand` recognizes MOVE/COPY/TRIM/EXTEND/ARRAY/FILLET/CHAMFER/BREAK/EXPLODE/HATCH and `CadView` has implementations. Must test geometric correctness, undo+redo, source/overlay selection and **saved** output; command alias is not proof of feature parity. |
| C-17 | OCS/UCS and Z-aware 2D placement | `DxfBlocks` deliberately ignores some elevation for plan display; full arbitrary 3D UCS and object coordinate transforms are not equivalent to AutoCAD desktop. |
| P-18 | Text / fonts / layers / layouts | SHX fallback, TTF and complex MTEXT supported in part; exact text metrics, substitutes, multiline write, dynamic attributes and XREF layers need fixture+pixel checks. |

## P2 – AutoCAD desktop 3D features missing or unproven
- Complete primitive solid creation (BOX, SPHERE, CYLINDER, CONE, TORUS, WEDGE)
- EXTRUDE, PRESSPULL, REVOLVE, SWEEP, LOFT
- UNION, SUBTRACT, INTERSECT, SLICE, SHELL, FILLETEDGE, CHAMFEREDGE
- UCS/dynamic UCS, 3D Gizmos, SECTIONPLANE, section extraction
- NURBS/surface editing, B-Rep topological editing, SAT/SAB persistence with licensing & format tests
- Advanced visual styles / texture / materials, GPU performance across huge solids
- Point cloud and PDF/DWF/DGN underlays with local resource resolution
- Batch publishing/plotting, data extraction and constraint/dynamic block authoring

These are development candidates, **not claimed broken menu buttons**. Some may only be appropriate for a desktop-grade tier or optional modular engine on Android.

## Test design – required before marking anything 'done'
1. **Fixture corpus**: genuine AutoCAD-exported DWG/DXF of versions, large/few objects, small/many objects, 3D-only ACIS, mixed 2D+3D, proxy, nested XREF, Arabic/Turkish text, ByLayer/TrueColor, paper viewports, OLE.
2. **Read audit**: source entity type + handle + extents count vs native + DXF + visual 3D with unsupported list, 2D/3D read-time parity and no false positive geometry.
3. **Write audit**: copy, edit, Save DXF, reopen and compare original + expected changed entities and untouched payload SHA. Require explicit user permission to edit original data.
4. **Performance**: 10 iterations each file/device, cold/warm load, 1st actual frame, 1st accurately colored frame, editable readiness, 3D FPS p50/p95, memory high-water, ANR count.
5. **Gate**: automated CI + real Android phone on signed compatible test APK; no `main` merge or automatic Cloudflare deployment for an unverified regression.

## Phased engineering sequence
- **Phase 1 – prevent false opening success/failure**: #242 3D-only read path, unsupported-object inventory and crash-safe reading.
- **Phase 2 – eliminate data loss**: #244 DXF save roundtrip tests; repair any discovered failures before attempting edited-DWG writing.
- **Phase 3 – first frame and fluid gestures**: #243 color parity, measurable native latency, 2D/3D draw acceleration.
- **Phase 4 – 2D AutoCAD parity**: XREF → DIM/MTEXT → HATCH/SPLINE → dynamic blocks → parametric tools.
- **Phase 5 – 3D solid parity and innovation**: editable B-Rep/solid output → sweep/extrude/boolean → advanced collaboration and AI-supported engineering review.

## Autodesk official feature reference
- AutoCAD desktop 3D solids/surfaces: https://help.autodesk.com/view/ACD/2026/ENU/?caas=caas%2Fdocumentation%2FCIV3D%2F2014%2FENU%2FfilesACD%2FGUID-44020C98-A94C-42C9-976B-D032FEF014E6-htm.html
- AutoCAD mobile external references: https://help.autodesk.com/cloudhelp/ENU/AutoCAD-Mobile-Help/files/File-management/ATTACH_EXTERNALREFERENCE.html
- AutoCAD desktop dynamic blocks: https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-LT/files/GUID-B6BF48AD-9408-4BE8-992D-4B7823F790F5.htm
- 3D modeling mobile support (Autodesk technical article, 2025-02-28): https://www.autodesk.com/support/technical/article/caas/sfdcarticles/sfdcarticles/3D-in-AutoCAD-mobile.html

**This document is a plan and a static audit. It does not certify every tool as malfunctioning or every bug fixed.**
