# MusaCAD — 18-Item CAD Engine Integration Gate (2026-10-13)

**Work branch:** `feature/cad-inventory-batch-20261013`
**Production:** `main` / live Cloudflare / installed APK are frozen while this batch is open.
**Test order:** make code changes first, integrate once, then invoke pull-request-triggered Android and Linux CAD workflows. Correct failures in the same branch and rerun until green. Do not confuse CI with physical-phone acceptance. Do not upload confidential user project DWGs to public GitHub artifacts.

## Feature inventory (only describe implemented code, never promise missing functionality)

| # | Domain | Current batch implementation | Remaining acceptance/gaps |
|---|---|---|---|
| 01 | 3D-only DWG/DXF | True-geometry branch to 3D viewer; ACIS proxy suppression | Actual .dwg / SAT / unsupported SAB real device; robust broken-file cleanup |
| 02 | 3D DXF edits | Preflight source coordinate slots; fail on ambiguous/missing tags | Real phone edited-vertex save/reopen; ACIS B-Rep still read-only |
| 03 | 2D DXF save | Full-precision values; stage output then structural validate before SAF destination is opened | Actual DXF geometry/objects roundtrip, Unicode/styles and interrupted SAF copy verification |
| 04 | Edited DWG writer | Opt-in legacy r2000–2004 native DXF→DWG writer, staged and reopened; **OFF in normal builds** | Evaluate fully compatible writer and legal/licensing constraints; no fake DWG export |
| 05 | 3D render speed | Bound edge draw buffer to 64 KiB; cull offscreen triangles/segments | Real device jank/FPS and large-solid tests, optional GPU redesign |
| 06 | First-frame DWG colors | Native vs DXF aggregate palette diagnostics and a suspicious-color warning | Fix the *specific* root cause on a reproducible DWG; pixel/color parity |
| 07 | Big-project opening/zoom | Stage timers and 128-frame Canvas CPU p50/p95; cull paper viewport entities | Real cold/warm loading, screen FPS and memory; no guaranteed 60FPS |
| 08 | Skipped/unread CAD entities | User-visible skipped counts; per-type bounded inventory; explicit raster IMAGE frame-only warning | Verify with AutoCAD side-by-side and add per-entity handle/position |
| 09 | Multiple concurrent projects | Existing up-to-four project tabs kept unchanged | Background preparing+switching, eviction/OOM and concurrent reference integrity |
| 10 | XREF DWG links | Detection of external attachment/overlay blocks, names/paths and unresolved warning | User-selected basic layer-0 DXF bind into a new DXF copy implemented; full arbitrary nested/layered XREF resolver still absent |
| 11 | Dynamic blocks | Ordinary blocks and MINSERT; INSERT/ATTRIB value inspection and safe copy editing | Visibility/action parameter editor and authentic dynamic grips |
| 12 | Layout/viewport | Skip model entities wholly outside viewport without deleting geometry | Nested viewport/layer VP frozen/rotation/print fidelity |
| 13 | Associative dimension | Existing DIM geometry readers/editors unchanged | DIMSTYLE constraint/reactive annotation, tolerance and source edits |
| 14 | Hatch/spline/MTEXT/fonts | Existing partial implementations retained | Full entity/Unicode/shape library write-read corpus |
| 15 | 2D tool selection | Conservative bounds prefilter before precise hit test, preserves nearest pick and large coordinate safety | Instrument real tap latency; command selection/undo/redo/write parity |
| 16 | OCS/UCS elevations | Existing 2D plan behavior retained | Orthonormal arbitrary UCS/WCS mesh transformation implemented; full per-entity AutoCAD UCS not implemented |
| 17 | B-Rep 3D solids | Real 3D read where ACIS SAT tessellates; unsupported solid proxy not considered decoded | Real ACIS/SAB edit/boolean/write engine |
| 18 | Advanced modeling | BOX, EXTRUDE, REVOLVE, LOFT, rising-Z SWEEP, box-only UNION/INTERSECTION/DIFFERENCE and horizontal SECTION added as real triangular geometry | **Generic arbitrary B-Rep** Boolean/SWEEP/LOFT, associative UCS editing, proper NURBS and GPU CAD parity remain open; current operations are limited triangular-mesh subsets |

## Final test plan — run after implementation batch is ready

1. Android lint + flavor/unit tests + native JNI compilation + source contracts; **release_approved=false**.
2. Linux host Debug and Release native DWG→DXF conversion, color palette and OLE payload round-trip.
3. DXF reading + staged write/reopen checks for geometry, text, layers, TrueColor, paper spaces, hatch, ACIS, OLE; SHA-256 original unchanged.
4. Fixture matrix with **DWG AC1018 / 2004**, **DWG AC1032 / 2018**, medium 4.7 MiB, larger 11.3 and 17.4 MiB project files. Files are available privately in the user's ChatGPT library; **not** committed to repo.
5. Actual Android real-device gestures, cold/warm opening, 2D/3D render timings p50/p95, memory peak, colors on *initial* frame and after zoom, Storage Access Framework destination cancellation/permission errors.
6. Merge/deployment only after CI+real-device acceptance. No claims of exact parity with full AutoCAD desktop until unsupported items 04/09/10/11/13/14/16/17/18 are genuinely implemented.

**Status:** A substantial subset of code from draft PRs #246–#255 has been combined in this batch; several of the 18 remain open. No final batched GitHub validation has run yet, intentionally. Passing earlier isolated PR checks is not proof the merged batch passes.

## 2026-10-13 Native modeling expansion (unmerged / CI not executed)

- `CadSolidKernel.java`, `CadSolidCommand.java`: closed-mesh BOX, EXTRUDE, REVOLVE, LOFT, rising-Z translation SWEEP, axis-aligned BOX Boolean and DXF 3DFACE export. Self-intersecting profiles and unsupported axis singularities are rejected.
- `CadUcs3d.java`: right-handed arbitrary UCS origin/axes transformation and inverse mapping; visible modeling UCS command.
- `CadMeshSection.java`: actual mesh/Z-plane edge intersections, DXF LINE export. Coplanar faces are deliberately excluded.
- `Mesh3dActivity.java`: real model actions run off the Android UI thread, selectable modeling parameters and staged DXF saving; imported ACIS source files are never overwritten.
- `CadXrefBinder.java`: user-selected external basic layer-0 geometric DXF attached as a new bound DXF copy, unsupported nested layers/blocks refused.
- `CadBlockAttributes.java`: INSERT/ATTRIB visible text editing in a source-preserving DXF copy. Proprietary dynamic block action/visibility graph remains absent.
- `NativeDwgWrite.java` and opt-in CMake path: old r2000–r2004 experimental LibreDWG DXF→DWG writer with re-read verification, disabled by default; later DWG versions are explicitly refused.
- Quality-gate tests created for these additions. **Do not claim test success until the final GitHub CI run actually completes.**

Explicitly outstanding: arbitrary ACIS/SAB B-Rep editing/writing and Boolean topology, modern DWG writer (2010/2013/2018), arbitrary XREF tree resolution/bind, action-driven dynamic blocks, full annotative DIMSTYLE, GPU renderer and full AutoCAD command parity.
