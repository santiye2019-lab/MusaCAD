# MusaCAD — Mandatory engineering directives

> Owner directive confirmed 2026-10-17. These rules are binding for every subsequent CAD development and PR in this repository. See [docs/UNIFIED_CAD_ENGINE_POLICY.md](docs/UNIFIED_CAD_ENGINE_POLICY.md), [issue #295](https://github.com/santiye2019-lab/MusaCAD/issues/295), and [docs/UNIFIED_CAD_ENGINE_INVENTORY.json](docs/UNIFIED_CAD_ENGINE_INVENTORY.json).

## Exactly ONE CAD engine, for every supported platform

- **ALL CAD engine functionality must ultimately live in a single portable, common C/C++ engine** exposed through a stable, versioned C ABI. It MUST compile for Android, Windows and iOS. One CAD object/data model, one set of geometry computations, one DWG/DXF read-edit-save implementation, one 2D/3D computation model, one undo/redo command semantics implementation.
- **No separate Java-only Android CAD engine or copied Swift/Windows CAD engine.** Existing Java geometry/data-model and DWG/DXF code is legacy migration debt, **never a permanent exception**, and must be tracked until actually ported. Do not claim that a feature is in the shared core merely because a Java helper exists or because its name appears in a command catalog.
- Platform-specific UIs, OS document pickers, file/print APIs, render adapters, JNI/Swift/Win32 FFI and lifecycle integration may remain platform-specific, but **do not duplicate CAD mathematical/semantic algorithms** in them. AI providers and UX may differ; CAD-backed engineering evidence, object identities and measurement semantics belong in the shared CAD engine.
- Refactor in verified increments; preserve data, existing app behavior, stable C ABI, DWG handles, coordinate systems, original file byte integrity and AC1032 compatibility goals.

## Fix failures; NEVER paper over them

1. Start at P0 data loss and source fidelity, then P1 and P2; inventory all engine features. For each feature, implement in the common core first and adapt to Android/Windows/iOS. Existing Java math may be temporarily retained *only during a tracked migration with matching reference fixtures*, never as the end state.
2. Run shared C tests on Linux, Windows and macOS, iOS arm64 cross-compile, Android lint/unit and relevant DWG read/write/reopen and source-preservation tests. Add real-device test evidence separately. Do not silently skip or remove failing tests, weaken assertions, or make up results.
3. **If ANY gate fails: read job logs, reproduce, fix the source bug or erroneous test, re-run affected and full relevant gates until green.** Do not abandon the task just because a test fails. Do not force-merge broken or stale PRs, and do not claim success while a required gate is pending.
4. For old PRs, compare their diff to the latest `main`, preserve newer changes and cherry-pick only verified relevant code; create clean incremental PRs.
5. **No APK creation, production/cloud deployments, storefront packages or app-store releases until the owner explicitly provides final approval.** Test-only iOS object compiles are not iPhone apps.
6. **AutoCAD 2026 parity is NOT established by compile success.** Report separately implemented feature scope, migration status and independently reference-certified capability families (currently no family has independent full certification). Never increase a percentage without evidence.

When constrained, continue incrementally with honest uncompleted-work status; never say all engine functions are ported unless the entire tracked inventory, all platform builds and tests demonstrate that outcome.
