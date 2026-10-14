# Rejected direct-child INSERT checkpoint experiment (2026-10-14)

**Status: REVERTED. Do not enable in Android, merge, or release.**

The draft-PR proof attempted to accelerate repeated nested-block traversal by
remembering a `Dwg_Object*` to the last failed depth-one leaf and restarting
the root `INSERT` at that pointer on the next page. Initial synthetic fixtures
and the public TS1.dwg passed, but **none exercised the shortcut**. Those green
tests were insufficient.

Private read-only tests exposed **duplicated geometry**:
- R2010 municipal drawing, 1,048,576 float words/page: safe indexed method
  yielded **1,284,764** primitive records / 10 pages; speculative shortcut
  yielded **1,364,526** records / 11 pages. Source DWG was not modified.
- Another ordinary engineering DWG, 8,192 floats/page: the new shortcut was
  used and failed byte-for-byte primitive/color/coordinate equality at page 4.

A reproducible **public** failure was then found in the pinned LibreDWG
`test/test-data/2018/Dynblocks.dwg`:
- Safe indexed implementation: **50 pages / 33,479 primitives**, v6 float
  stream bit-for-bit equal to the complete non-truncated scene.
- Unsafe direct-child shortcut: activated at top-level cursor `33:498`,
  then **mismatched the baseline on page 1** with a 4,096-float page budget.
- No private drawing or coordinates need to be uploaded to reproduce it.

The root problem is that a direct child pointer plus a leaf ordinal **does not
encode a sufficiently complete expansion context** in a recursively
transformed/nested block graph. Do not reinstate this naive shortcut. A valid
replacement must preserve block-instance transforms, MINSERT row/column
placement, nested parent state, source-order traversal and atomic leaf
boundaries, and must support arbitrary page re-fetch from the catalog.

## Protection

The exact upstream `2018/Dynblocks.dwg` acceptance is now part of
`.github/workflows/cad-r2000-fidelity.yml`. Every generated 4,096-word page
is compared with the complete v6 stream (coordinates, record order and color
metadata), not merely counted. The CI also rejects the removed shortcut's
entry point in `dwg_bridge.c` until a fully validated replacement exists.

The previous **indexed top-level model entity** optimization is preserved.
It accelerates access to a known top-level cursor but still replays leaves
within certain deeply nested inserts. The current Android viewport renderer
remains opt-in; the production APK and `main` are unchanged.

## Required next milestone

Implement an **explicit complete traversal/transform stack** (not merely a leaf
pointer) with a bounded session checkpoint and correct random-access fallback.
Prove it using public Dynblocks (50 pages), TS1, synthetic 8,193 objects,
single-too-large-leaf fail-closed, and real read-only engineering drawings
before attempting any Android activation.
