# MusaCAD — Mandatory engineering directives

> Owner directive confirmed 2026-10-17. These rules are binding for every subsequent CAD development and PR in this repository. See [docs/UNIFIED_CAD_ENGINE_POLICY.md](docs/UNIFIED_CAD_ENGINE_POLICY.md), [issue #295](https://github.com/santiye2019-lab/MusaCAD/issues/295), and [docs/UNIFIED_CAD_ENGINE_INVENTORY.json](docs/UNIFIED_CAD_ENGINE_INVENTORY.json).

## AutoCAD 2027 functional equivalence supersedes 2026 — owner decision 21 October 2026

- **The active comparative product goal is AutoCAD 2027, not AutoCAD 2026.**
  Keep the existing 51 capability family IDs unchanged and track five
  verified 2027 feature deltas in
  [docs/AUTOCAD_2027_TARGET.json](docs/AUTOCAD_2027_TARGET.json).
- Treat prior 2026 matrix, acceptance records and 2026 command/catalog
  tests as historical compatibility baselines. Do NOT blindly rename
  earlier test output, invent 2027 evidence or conflate existing code
  with certified 2027 parity. Report certified 2027 families separately:
  initially 0/51 pending real independent reference acceptance.
- Preserve the fixed 25 common-core gates and 16 AC1018 ingest milestones
  as independent scoped metrics. Do not increment percentages for a
  planning/target revision or partially implemented features.
- The external AutoCAD 2018-family DWG wire format (AC1032) persists in
  AutoCAD 2027. Original MusaCAD MC1032 (.mcad) must stay separate.
- All existing source-integrity, fail-closed, independent C/C++ one-core
  and no-APK-until-explicit-owner-approval rules remain binding.

## Verified sequential development — owner instruction 18 October 2026

- Within an active development session, after each independently verified P0
  engine increment: repair all failing native/Android/CAD tests, confirm all
  five latest-commit required workflows green, merge to main only with an
  expected-head-SHA lease, then move directly to the next independently
  verifiable engine increment rather than asking for routine reapproval.
- Never hide a failed test, suppress a regression check, silently drop
  unsupported geometry, leave a half-written source file, or claim that a
  partially implemented feature is complete. Keep mobile, Windows and iOS
  geometry semantics in the same portable MusaCAD-owned C/C++ core.
- No APK, production release, Cloudflare deployment or user-facing update
  of a phone installation until explicit owner approval. Do not claim
  development continues while the conversation is inactive; only actual
  executed GitHub commits/tests/merges count as completed progress.
- Certified AutoCAD 2027 functional equivalence remains separately tracked
  from completed implementation work, with no unverified percent increases.

## MC1032 native file-format naming — binding owner decision (18 October 2026)

- MusaCAD's **OWN** native binary document format and on-disk signature is
  **MC1032**, with extension `.mcad`. Define and maintain its own format
  specification in [docs/MC1032_BINARY_FORMAT.md](docs/MC1032_BINARY_FORMAT.md).
- **Do not rename an Autodesk `AC1032` DWG header to `MC1032`.** That is a
  different external file format; merely changing its magic bytes would be
  incompatible and misleading. AutoCAD/DWG interoperability needs a genuinely
  first-party DWG decoder and encoder, separate from MC1032.
- All MC1032 read, write, object validation, source handle semantics,
  checksums and edit semantics belong to the single portable C/C++ CAD core.
  Android JNI, Windows and iOS expose that same ABI; platform-specific file
  dialogs, native storage and UIs are thin adapters only.
- New MC1032 capabilities must be cross-platform, round-trip tested, and
  reject unsupported entities rather than silently losing layers, solids or
  metadata. Existing v1 LINE/CIRCLE/ARC/POINT is a tested subset, NOT
  the end-state engine or AutoCAD 2026 parity.
- Never create an APK or release without explicit owner approval.

## Fully independent first-party MusaCAD engine — mandatory owner decision, 18 October 2026

- The **released CAD engine must contain no LibreDWG, ODA, third-party DWG/DXF parsing/serialization/geometry kernel or runtime fallback**. MusaCAD must own the DWG binary reader, DWG binary writer, DXF reader/writer, object model, geometry, 2D/3D modeling and editing, commands, and undo/redo in common C/C++ source.
- Existing `app/src/main/cpp/` LibreDWG bridge is **legacy migration debt**, not the MusaCAD engine and not acceptable as a permanent dependency. Do not add new calls to it, move it into `core/native/`, or label its output as MusaCAD-native decode. **Owner superseding instruction 18 October 2026:** remove LibreDWG from the existing Android runtime IMMEDIATELY. DWG-open must explicitly fail closed until a genuine first-party decoder passes independent acceptance, without modifying the source file. Do not claim that losing DWG-open support constitutes complete compatibility.
- Build first-party `core/native/` with **no external CAD decoder/encoder libraries**, on Linux, Windows, macOS and iOS cross-compile. Tests must fail if independent core source starts including/linking third-party CAD code.
- **Final DWG availability condition (distinct from immediate dependency removal):** verify genuine binary AC1015–AC1032 decode AND encode with original-object IDs, geometry, colors, layers, blocks, dynamic content, XREF, annotations, layouts and 3D/ACIS semantics or explicit loss rejection; independently reopen and compare. The existing Android runtime LibreDWG bridge must already be disabled; host-only reference tooling is removed after equivalent independent tests exist. Unsupported objects must fail closed, never disappear silently.
- Do not confuse a six-byte AC1032 header recognition, synthetic DXF, a geometry preview, or a compiled stub with a native DWG reader/writer or AutoCAD 2026 equivalence. Treat all missing functions as tracked work, not as complete.
- No APK or deployment until the owner's explicit final approval.

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
6. **AutoCAD 2027 parity is NOT established by compile success.** Report separately implemented feature scope, migration status and independently reference-certified capability families (currently no family has independent full certification). Never increase a percentage without evidence.

When constrained, continue incrementally with honest uncompleted-work status; never say all engine functions are ported unless the entire tracked inventory, all platform builds and tests demonstrate that outcome.
