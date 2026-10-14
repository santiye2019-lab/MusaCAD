# Exact nested leaf-instance context — experimental, 14 October 2026

**Status: verified on public Dynblocks and three private local DWGs, but not yet
a demonstrated mobile performance improvement. No production release.**

## Why the previous direct-child optimization was rejected

The prior direct-child pointer/leaf-number approach duplicated drawing records
in nested INSERT structures. It has been removed. See
`docs/DIRECT_BLOCK_RESUME_REGRESSION.md`.

## Implemented alternative

`musa_scene_collect_root_leaves` traverses one model-space INSERT, MINSERT or
dimension expansion using the same code as the renderer. Instead of emitting
geometry, it collects an *instance* for every nested leaf, in exact source order.

Each entry preserves:
- Original Dwg_Object pointer, valid only during the parent DWG session
- The complete 2D parent affine transform (6 doubles)
- Original nested depth
- ByBlock ACI/TrueColor and inherited layer ACI/TrueColor

This distinguishes different transformed MINSERT instances even if they share
the same underlying leaf Dwg_Object pointer. New pages emit each complete leaf
using the saved context and retain the existing whole-leaf rollback on overflow.
The original indexed nested paging method remains the fallback.

Hard guard: **65,536 leaf instances for one cached expansion**. Failure or a
larger expansion causes safe ordinary nested replay, not truncated success. The
cache is non-owning, scoped to one live DWG session and freed on close. Only
one top-level expansion is cached at a time. The v6 primitive stream and
production MainActivity default render path are unchanged.

## Public test results

[Focused R2000 proof](https://github.com/santiye2019-lab/MusaCAD/actions/runs/38550019310)
PASSED. Public pinned LibreDWG 2018/Dynblocks.dwg:
- 50 bounded pages and **33,479 primitive records**, byte-for-byte identical
  to the complete legacy v6 stream, including geometry, order and color.
- Logs show multiple actual uses of the new context index, rather than merely
  passing via fallback. Pinned 2000/TS1.dwg also exercises it.
- Single oversized leaf still must fail closed, and synthetic 8,193-primitive
  fixture remains equal.

## Read-only engineering file acceptance (not uploaded)

Three project DWGs were tested locally with the current model and comparison:
| File | Float words/page | Primitives (both) | Indexed seconds | Context seconds |
|---|---:|---:|---:|---:|
| R2010 Karagümrük | 1,048,576 | 1,284,764 | 0.61 | 0.63 |
| R2018 Yeniçeşme | 65,536 | 542,222 | 1.11 | 1.15 |
| Silivrikapı Kafe | 8,192 | 29,323 | 0.06 | 0.06 |

Same number of pages in both modes: 10, 50, and 33 respectively. Sources
retained identical SHA-256 before/after. These are single-run Linux host
measurements, not Android FPS or proof of a speed gain.

To strengthen coverage for drawings whose old full-scene output is truncated,
the host-only acceptance now supports opt-in exclusive local serialization
of the concatenated float payloads, followed by externally computed SHA-256.
No source bytes are uploaded. The CI checks this in a public 50-page case.

## Remaining gates

- Finish full bytewise stream comparison for private DWGs, especially the
  Karagümrük input where the legacy full-scene baseline was truncated.
- Reduce nested traversal *without materializing all leaf contexts* when
  blocks exceed the strict 65,536-instance memory guard.
- Properly prioritize and reload visible pages in the phone UI, with source
  identity guards and no ANR or OOM.
- Real Android device tests: first-paint true colors, pan/zoom FPS, frame
  latency, heap/native memory, cancellation and screen/device rotation.
- Independent CAD viewer verification of geometry/blocks/XREF/3D fidelity.

Do not merge to main or release APK, claim AutoCAD parity, or enable DWG
Save As on this host-only proof.
