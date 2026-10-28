# MC1032 third-party GPL replacement gate

Status: **NOT CLEARED FOR PROPRIETARY DISTRIBUTION**. This is a provenance audit, not a license change.

## Confirmed baseline

- Root `LICENSE` contains GNU GPL version 3.
- `core/native/include/musacad_dwg_ac1032_scene_index.h` and `core/native/src/musacad_dwg_ac1032_scene_index.c` declare `SPDX-License-Identifier: GPL-3.0-or-later`.
- `core/native/CMakeLists.txt` builds a portable C static library used by Android; the build graph needs dependency-level review.
- A GPL header alone does **not** establish LibreDWG copying, and deleting a header does **not** remove copyright obligations.

## Mandatory replacement protocol

1. Inventory **all** `core/native` C/H files, generated sources, vendored files, build inputs and transitive linked dependencies. Record exact path, authorship, commit origin, license evidence and any third-party source overlap.
2. Classify each file as `OWNED_RELICENSABLE`, `THIRD_PARTY_GPL_REPLACE`, or `UNKNOWN_HOLD`. Never presume `OWNED_RELICENSABLE` from a filename or a copyright header alone.
3. For `THIRD_PARTY_GPL_REPLACE`, implement the required behavior independently from public DWG format descriptions and independently authored tests. Do not copy or mechanically paraphrase GPL code. Retain legally required notices until the affected code is actually removed.
4. Validate original R2018 DWG fixtures, BLOCK/INSERT ownership, LINE/curve/text/layer behavior and Android/iOS/Windows C11 compilation. Preserve the current reader while replacements are developed on a branch.
5. Change SPDX notices **only after** verifying ownership and obtaining permission from all relevant rights holders; update root licensing only after the full dependency audit.

## Release blockers

- [ ] Complete file-level provenance inventory.
- [ ] Confirm ownership/relicensing rights for every core source.
- [ ] Replace every confirmed third-party GPL implementation.
- [ ] Check transitive native dependencies and build artifacts.
- [ ] Pass real DWG regression and portability checks.
- [ ] Approve and document final licensing decision.

No claim of GPL-free status is authorized until every blocker is closed.

## Initial build-graph findings (2026-10-28)

- `core/native/CMakeLists.txt` explicitly builds a **single** `musacad_core` static C11 library from DWG AC1015/AC1018/AC1032, DXF, geometry, 3D and `musacad_mc1032*` translation units. All those units must be audited; examining only `musacad_mc1032.c` is insufficient.
- Inspected GPL-SPDX source samples: `src/musacad_mc1032.c`, `src/musacad_dwg_ac1032_handles.c`, `src/musacad_crc16.c`, `src/musacad_dwg_ac1032_scene_index.c`, `include/musacad_mc1032.h`, `include/musacad_dwg_ac1032_scene_index.h`. **Ownership is still UNKNOWN**, not confirmed third-party copying.
- `include/musacad_mc1032.h` currently describes MC1032 as a MusaCAD-owned **binary document format, not DWG/AC1032**. This is a naming collision with the product's intended common CAD core: do not conflate this custom format with the native DWG reader during replacement.
- `src/musacad_mc1032.c` comments assert independence from LibreDWG/ODA, but a source comment is not sufficient proof of provenance.
- The inspected `src/musacad_crc16.c` contains a small CRC-16/ARC loop. This standardized algorithm alone is not evidence of third-party GPL copying.

**Next actionable gate:** enumerate every `add_library(musacad_core ...)` source and each local included header, correlate earliest introducing commits/authors and any vendored upstream lineage; only then label individual files for clean-room replacement or authorized relicensing. No license headers have been removed.
