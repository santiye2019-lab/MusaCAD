# MusaCAD2 legacy-code transfer procedure

Every legacy-to-MusaCAD2 transfer requires a **per-file and, when mixed, per-function provenance review** before any code is copied.

## Gate 1: inventory
Record original path, source commit SHA, authors/contributors, copyright owners, file and dependency licenses, transitive dependencies, and evidence links in TRANSFER_LEDGER.csv. Mark unverified items QUARANTINED; do not copy them.

## Gate 2: rights and dependency review
Check all contributions and third-party fragments, including generated code, tests, assets, and linked libraries. GPL-3.0, GPL-3.0-or-later, AGPL and incompatible copyleft dependencies are not admitted. Removing license headers, renaming symbols, or superficial rewrites do not establish rights. GPL-labelled first-party code requires explicit confirmation of all necessary relicensing rights before reuse.

## Gate 3: approved migration
Only transfer files or functions whose rights and dependencies have been verified and whose reuse is authorized. If provenance is uncertain or the license is incompatible, leave the old code untouched and independently implement the behavior from permitted specifications, recording that approach. Never copy the incompatible implementation as a starting point.

## Gate 4: verification
Run standalone MusaCAD2 builds/tests on Linux, Windows and macOS, plus format fixtures and regression tests relevant to the migrated behavior. Keep main unchanged and changes isolated to the MusaCAD2 branch until explicit merge approval.

## Gate 5: review and release
A reviewer verifies ledger evidence and CI results. No claim of GPL-free status or commercial relicensing clearance is made until all code, assets and dependencies have passed a complete audit.

**Decision values:** QUARANTINE / REJECT / REIMPLEMENT / APPROVE. **Status values:** PENDING / IN_REVIEW / VERIFIED / TESTED.

## GPL functionality: independent replacement, not derivative code

When a useful working implementation is GPL-licensed or its rights cannot be verified, **do not copy, adapt, translate, or superficially rewrite that implementation**. Preserve the desired functionality as a behavioral requirement, not its source expression. Document inputs, outputs, supported cases, and independent acceptance tests; consult lawfully available format specifications and clean test fixtures. Prefer a separate implementer who has not studied the restricted implementation where practical. Develop original algorithms, data structures, interfaces, and error handling; validate functional equivalence and improvements without transplanting restricted code. Record the replacement as REIMPLEMENT, with evidence of independent authorship and tests. This procedure does not itself certify legal independence; rights review remains mandatory.
