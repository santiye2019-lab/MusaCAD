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
