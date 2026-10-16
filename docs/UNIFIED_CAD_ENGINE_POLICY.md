# MusaCAD — One engine on Android, Windows and iOS

**Binding owner decision, 17 October 2026** • Issue [#295](https://github.com/santiye2019-lab/MusaCAD/issues/295) • Executable instructions: [../AGENTS.md](../AGENTS.md)

## Target, without exceptions for engine semantics

Exactly one CAD engine, with versioned and memory-safe portable C ABI. All CAD-specific computation, DWG/DXF record interpretation, source identity, units, layer/layout/block/XREF object semantics, editing and command transactions, 2D/3D geometry, selection, OCS/UCS, parametric model state, mesh/B-Rep/topology, geometric measurements, dimensioning/hatch rules, serialization, undo/redo and input validation are performed by this common engine.

Android JNI, Windows C ABI, and iOS Swift/C wrappers are *thin adapters* to that engine. Platform UI, accessibility, file dialogs, printing, window/view state and GPU API integration differ but must not contain a second copy of CAD functionality. AI/cloud providers can be OS-specific, but CAD evidence and engineering measurement semantics must be shared.

Legacy Android Java CAD calculators are temporary migration debt, not an exception. At present the existing project contains numerous Java CAD classes; it is **incorrect** to say all CAD capabilities are already shared. Existing Java fallbacks can stay only while the corresponding common implementation is being proven and integrated, and must be eliminated or reduced to adapters at completion.

## Port every capability, in dependency order

| Priority | Capability families that must end up in common core | Independent acceptance |
|---|---|---|
| P0 | Source integrity, handling of malformed inputs, original DWG/DXF byte preservation, safe atomic-ish staged editing and save, object identity, metadata, 3D-only drawings | Actual DXF/DWG byte fixtures, zero invented B-Rep or proxy geometry, save→reopen and failure modes |
| P0 | AC1015 through AC1032 read/write support and loss audit (modern AC1032 writer is not yet certified); 2D/3D first frame fidelity, full color/layer/block semantics, large file paging | Real binary DWGs, handle/layer/XREF/coordinate preservation; p50/p95 on physical devices |
| P1 | LINE/ARC/CIRCLE/ELLIPSE/NURBS/POLYLINE/SPLINE/HATCH/MTEXT/DIMENSION/attributes, native color, UCS/OCS coordinate transforms and drawing queries | One common fixture suite across OSes; correct WCS/OCS and serialized output |
| P1 | TRIM/EXTEND/OFFSET/FILLET/CHAMFER/COPY/MOVE/MIRROR/ROTATE/SCALE/ARRAY/BREAK, grips, snaps, selection and command undo/redo, BLOCK/MINSERT/XREF/layout/viewport | Matching before→edit→write→reopen entity identities and geometry |
| P2 | MESH/B-Rep/ACIS/SAT/SAB, topology, volume, surface, EXTRUDE/SWEEP/LOFT/REVOLVE/BOOLEAN/SECTION, 3D gizmo and 3D UCS | Common geometry and strict invalid/missing-surface errors; no fabricated exact solids |
| P2 | CAD-aware engineering evidence, entity-linked discipline classification, object quantities and coordinated corrections | Same deterministic evidence/units and CAD object IDs across platforms |

Each row contains many independent capabilities; **a passing example does not certify the entire family**. The existing 51-family AutoCAD 2026 reference acceptance is still separate from migration scope.

## Non-negotiable development and promotion procedure

1. Inventory all files and actual operations, identify current owner in Java/C and classifiable adapter vs computational engine. The conservative initial file-level candidate inventory is stored in `docs/UNIFIED_CAD_ENGINE_INVENTORY.json`. *Class presence is not proof of implementation, coverage, or equivalence.*
2. For every engine feature under migration, produce a portable C/C++ operation with stable C ABI and deterministic error handling (no hardcoded Android or Win32 SDK data in the core). Retain old Java semantics only as a temporary differential test oracle; remove duplicate algorithm after verifying the adapter.
3. Cross-compare actual geometry and source identities with common golden fixtures, DWG→DXF↔DWG/roundtrip where supported, malformed/truncated inputs, empty files, XREF/ACIS unsupported paths, large WCS coordinates and memory bounds. Test C on Linux, Windows, macOS; iOS ARM64 object build; Android lint/unit; actual handset and PC acceptance later.
4. If a test fails, inspect logs, isolate root cause, correct code/test and rerun. Never bypass, delete or fake a test to get a green badge. A blocking failure keeps the PR out of main, **not off the roadmap**.
5. After checks pass, review diff against current `main` and merge only independently proven changes. No blind merging of stale PRs. No APK or production deployment without explicit owner approval.
6. Report exact gates/PR numbers and limitations; independent AutoCAD 2026 reference certification stays 0/51 until objectively verified, regardless of new files or code refactors.

## Migration inventory interpretation

The baseline inventory enumerates Java source modules potentially touching engine or adapters, not completed functions. Each is reviewed as `needs_core_audit` or `platform_adapter_candidate`. A platform adapter candidate can contain accidental CAD math and is **not automatically certified**. An engine module is considered fully migrated only after its concrete algorithms and public contract are in portable C, all adapters call those functions, duplicate per-platform implementations are eliminated and multi-OS/physical-device acceptance passes.

The inventory must be revisited whenever Java/C source code changes, and a newly introduced Java CAD candidate must be explicitly audited before the PR is eligible to merge. Its existence never counts as a completed core feature.
