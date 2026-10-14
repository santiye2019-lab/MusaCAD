# MusaCAD: nested BLOCK streaming acceptance (experimental)

**No release enablement.** This proof is hosted on a draft PR; it does not
change production Android first paint, CAD file writer, releases or the main
branch.

## What is implemented

`musa_scene_build_page_nested()` returns one bounded native scene with a
continuation cursor `(top_level_entity, nested_leaf)`. When a top-level
`INSERT`, `MINSERT` or dimension expands recursively and exceeds the page,
fully emitted child leaves are committed. The cursor resumes at the first
unemitted whole child leaf. All nested block coordinates, colors and transforms
are recalculated using the existing emitter; no partially emitted child is
allowed. A single leaf that cannot fit returns `-4`, **not** false success.

Old `musa_scene_build()`, `musa_scene_build_page()`, production `sceneNative()`
and `NativeCadEngine.previewPage()` remain unchanged. The new Java
`previewNestedPage()` is opt-in and **not wired into `CadView`**.
Both producer and consumer must preserve the full two-part cursor.

**Critical limitation:** This first version re-traverses earlier nested leaves
when producing a later page. It is not optimal for massive nested blocks.
A future explicit iterator stack and viewport-aware spatial streaming are
required to meet smooth pinch/pan performance.

## Acceptance

The pinned redistributable LibreDWG 2000/TS1 DWG was read with a tiny 256-float
page; its block expansion actually crossed a page boundary:
the log had `next=10:2` and the next page began at `top=10 leaf=2`.
All 82 emitted primitives matched the untruncated legacy scene count.
The CI workflow verifies a nonzero nested-leaf continuation and rejects
single-leaf oversize instead of silently truncating.

Private host-only acceptance on a real ~4.7 MB R2010 municipal DWG
(*not* uploaded to GitHub, no geometry or sensitive layer names logged to PR):
- Baseline limited scene: 1,206,786 render primitives, `status=1` (truncated).
- Nested 1,048,576-float pages (4 MiB float buffer): **10 pages** and 1,284,764
  render primitives, matching the prior 4,194,304-float bounded host traversal.
- Local Linux duration: 0.96 s, peak RSS 216,944 KiB (~212 MiB).
- Private source SHA-256 unchanged before/after.
- These are **host only** figures, not Android frame times or complete DWG fidelity.

## Remaining release gates

1. Prove every supported primitive record, its geometry/attributes and correct
   order across page boundaries (count equality alone is insufficient).
2. Replace replay with a truly resumable nested iterator/explicit block stack,
   and impose CPU time/instance limits for pathological recursive blocks.
3. Wire the opt-in pages into bounded on-screen tile caches, prioritize only
   visible geometry, and avoid long work on Android's UI thread.
4. Test with full-color 2D drawings, large blocks, XREFs, annotation/3D objects
   and independent CAD reader comparisons.
5. Physical Android acceptance: initial true colors, panning/zooming frame
   times, app responsiveness, OOM avoidance and on-device source integrity.

Until these pass: **no main merge, APK release or public DWG Save As switch**.
