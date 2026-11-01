# Independent viewport layer evidence — 2026-11-01

- Scope: original MusaCAD branch, `core/firstparty/dwg1032/`. Not `MusaCAD2/`.
- Files: `musa_dwg1032_viewport_visibility.[ch]` and dedicated synthetic tests.
- Origin: independently authored MIT-licensed C11, from ordinary sorted-array binary search, Boolean composition and checked pointer ranges. No LibreDWG, GPL/AGPL/LGPL implementation, historical native-core code, or unknown-origin CAD decoder was copied, adapted or linked.
- Behavior: combines **caller-verified, already decoded** entity on/off state, globally sorted layer on/thawed state, and sparse per-viewport freeze overrides. Missing viewport overrides are not invented. Unknown layers, invalid flags, duplicate overrides and late malformed entries fail atomically.
- Tests: two independent viewport instances, default viewport, invalid layer/flag/alias cases, and 20,000 entities against 1,024 layers with 128 overrides compared with a separate linear reference. CTest name: `firstparty_dwg1032_viewport_layer_evidence`.
- Boundaries: does **not** parse AC1032 viewport objects, section/page compression, real DWG layer flags, block owner handles, entity records, TEXT/MTEXT or final render visibility. A caller must still prove source provenance and combine the local result with authenticated INSERT/owner context. No Android JNI import is enabled.
- **License integration blocker found during review:** `app/src/main/cpp/CMakeLists.txt` currently links `musacad_core` from `core/native` into Android `musacad_dwg`, and examined files under `core/native/src` carry `SPDX-License-Identifier: GPL-3.0-or-later`. Therefore the existing Android native library must **not** be represented as GPL-free or independently cleared for proprietary distribution. This commit deliberately does not alter or copy those sources, disable existing 3D features, or link this MIT target to the legacy app. A separate source-origin/legal audit and replacement of copyleft-linked dependencies are required before an independent production release.
- Validation: CI and actual licensed AC1032 fixtures are separate gates; synthetic C11 tests cannot prove real DWG compatibility.

## Viewport + owner-forest composition (2026-11-01)

- Added `musa_dwg1032_viewport_owner_visibility` to compose authenticated, post-decode per-viewport layer flags with inherited owner visibility. A hidden ancestor cannot be re-exposed by a thawed child. No heap allocation; all advertised buffers, including scratch tails, are checked for aliasing before writing output.
- Fail-closed tests cover distinct viewport states, hidden parents, a late ownership cycle, dangling handles, unknown layers, invalid flags, mutated object indices, output aliasing and a 20,000-object owner chain. The existing CTest target executes these cases on each supported CI host.
- This is not a DWG viewport parser, final INSERT instance renderer, AC1032 decompressor, real-file acceptance test or Android integration. Existing GPL-linked Android native dependency remains a release blocker; no license-clearance claim is made.
- All added C11 logic and synthetic test fixtures were independently authored for this repository. No third-party CAD implementation was copied, adapted or linked.

## Per-INSERT viewport layer-zero inheritance (2026-11-01)

- Added first-party C11 `musa_dwg1032_viewport_insert_visibility` to recompute direct child visibility for each concrete INSERT and selected viewport. The definition's layer-zero on/thawed/viewport-frozen bits are deliberately ignored only for layer-zero children; their visibility is inherited from the proven INSERT instance. Nonzero child layers retain their own verified global and viewport visibility, combined with owner-only flags.
- The routine validates all layer and sparse override tables, layer-zero identity, every child handle and flag, and output aliasing *before* publishing any child result. 20,000-child independent linear-reference checks, multiple viewport states, hidden instances and malformed late entries exercise its all-or-nothing behavior.
- This is a predecoded composition aid only. Nested INSERTs require per-instance calls; authenticated source provenance, original DWG read, entity decoding, final scene rendering and independent real DWG fixtures remain incomplete. The Android GPL-linked native library remains a release blocker. No third-party CAD implementation was copied or linked.

## Transactional instance color + viewport composition (2026-11-02)

- Added `musa_dwg1032_instance_attributes.[ch]` and a dedicated CTest that jointly compose each concrete INSERT's ByLayer/ByBlock/TrueColor and instance-local viewport visibility. The caller must supply authenticated, post-decoded source colors, sorted layers and viewport overrides. Both published output arrays remain unchanged if either source color or a later visibility check fails; caller-owned disjoint scratch is used.
- Synthetic positive/negative tests cover two independent instances, layer-zero color and visibility inheritance, viewport freeze, hidden INSERTs, unresolved ByBlock rejection, buffer aliases, invalid scratch capacity, and a 20,000-child deterministic crosscheck.
- Provenance: original MIT C11 orchestration of other original independent MusaCAD helpers; no GPL, LibreDWG, proprietary CAD implementation or third-party decoder was copied or linked. This remains **post-decode only**, not a verified AC1032 entity decoder, section/page decompressor, actual INSERT geometry renderer, Android JNI integration, or real-file proof. The GPL-linked existing Android core remains an unresolved licensing blocker.
