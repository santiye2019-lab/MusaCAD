# AC1032 2D render-chain recovery — 2026-11-02

**Repository:** MusaCAD (not MusaCAD2). **Scope:** evidence-based ownership/integration inventory, not a claim of full DWG rendering. Do not enable normal DWG import, delete legacy code, merge PR #705 or change main on the strength of this inventory.

## What already exists — do not reimplement blindly

| Stage | Existing implementation / evidence | Verified scope | Current production boundary |
| --- | --- | --- | --- |
| Bounded binary reads, object spans/maps, owner graph, source layer/color and viewport inputs | `core/firstparty/dwg1032/{src,include,tests}` (MIT-only standalone C11 target) | Isolated bounded/decoded-input contracts and synthetic regressions; some stages still require authentic AC1032 source interpretation | Not a complete DWG-to-scene reader; no production JNI linkage |
| Nested INSERTs, affine placement, inherited layer 0, BYBLOCK/BYLAYER, text helpers, candidate culling and large-sheet paging | `core/firstparty/scene2d/{src,include,tests}` (MIT-only standalone C11 target) | Predecoded scene graph and viewport semantics; synthetic cross-platform tests | Consumes already decoded objects; no AC1032 bytes or production JNI linkage |
| Genuine AC1032 diagnostic source previews | `app/src/main/java/com/musa/cad/FirstPartyDwgPreview.java` and `app/src/main/cpp/ac1032_*_jni.c` | Opt-in **partial** preview, including direct primitives and separate INSERT/CIRCLE evidence; some colors explicitly diagnostic, not source paint | Not a full editable document; provenance/license of existing native files must be reviewed before reuse |
| Normal Android DWG open | `app/src/main/cpp/firstparty_dwg_unavailable_jni.c` | Explicit IOException/no conversion; preserves original DWG bytes | Intentionally blocked until complete first-party document semantics are independently accepted |
| 3D models and display | Existing `core/native` and Android 3D code, tracked in `docs/UNIFIED_CAD_ENGINE_INVENTORY.json` | Prior partial 3D work must be preserved and audited separately | Not a reason to claim complete ACIS/B-Rep or AutoCAD 2027 parity |

## The missing link (not another LINE/ARC/CIRCLE implementation)

The standalone first-party DWG cursor/object modules and the standalone first-party 2D scene evaluator are **not linked together**, and neither is the production Android DWG open path. A green unit test for any of these modules is not evidence that one real DWG was decoded into a complete model-space scene. The normal-open refusal is intentional and must remain until source-authenticated, full-document semantics exist.

**Dependency order for the next work:** independently verify real AC1032 section/page/object decoding → construct authoritative model/layout and block-owner membership → resolve per-entity and layer/viewport visibility plus source color → feed a single source-preserving C11 scene contract → render in Android via a thin JNI adapter → compare actual DWG reference frames and object counts (including text, blocks, colors, layers, large drawings). Unknown source semantics must reject promotion to normal open, not silently omit objects.

## Duplicate and license rule

- The independent MIT `core/firstparty/dwg1032` and `core/firstparty/scene2d` targets are the **candidates** for future common-core ownership. Similar historical `core/native`, Java and diagnostic JNI functions are not automatically interchangeable or safe to copy.
- Historical `core/native` and `app/src/main/cpp` include **GPL-3.0-or-later-marked files** (for example, `musacad_dwg_ac1032_display_readiness.c` and `firstparty_dwg_unavailable_jni.c`). **Do not copy, adapt, link, relabel or remove their code as a shortcut.** Review provenance and interfaces before any clean-room replacement.
- Keep the old 3D implementations and tests intact until independently verified replacements exist. Remove an obsolete implementation only after confirming consumers, behavior, tests and source ownership; do not equate matching names with redundant code.
- PR numbers are not feature-completion counts. Report independently proven end-to-end capabilities and CI by exact SHA, not cumulative PR totals.

## Current bounded performance increment

`fca34aecb32dea56925a524f1853557db8908626` reduces caller scratch required by the standalone viewport candidate collector from `count` entries to `min(count, capacity)` entries, without changing accepted-hit ordering or output rollback. A 20,000-object synthetic sparse viewport test covers the smaller buffer and zero-hit case. This is **not** real-DWG loading, nor a measured Android device performance claim. Check CI for the exact SHA before calling it verified.

**Next acceptance blocker:** a license-clean, source-authenticated real-AC1032-to-first-party-scene adapter with full ownership, layer and text evidence. No fabricated source colors, block children or silent omissions.
