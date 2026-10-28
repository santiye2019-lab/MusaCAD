# MusaCAD vs MusaCAD2: architectural and ownership boundary

**MusaCAD** is the existing reference application and may contain third-party or GPL-licensed source. It is used for requirements discovery, behavior comparison, and independently prepared acceptance tests. Its presence in the same GitHub repository does not authorize copying code.

**MusaCAD2** is the new independently authored CAD implementation. Its source, algorithms and core components must be developed under verified ownership rights, with no copied or derivative GPL source. Features that work in MusaCAD should be independently reimplemented, then improved and validated against lawful specifications and independent test fixtures.

## Mandatory boundary
- Do not import, vendor, link or adapt GPL/AGPL source or incompatible dependencies into MusaCAD2.
- Treat old MusaCAD code as a reference system, not as an implementation template for restricted source.
- Reuse existing code only if provenance and all necessary rights have been positively verified; otherwise write a genuinely independent implementation.
- Track decisions and evidence in migration/TRANSFER_LEDGER.csv and follow migration/TRANSFER_PROCEDURE.md.
- Keep the existing main branch unchanged until explicitly authorized.
- Target a portable C11 core for Android, iOS and Windows, with independently verified DWG read/write capabilities.
- No claim of complete ownership or GPL-free release until source, dependency, contributor and artifact audits are complete.
