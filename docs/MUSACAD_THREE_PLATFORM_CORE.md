# MusaCAD — One CAD engine, three platforms

## Product architecture decision (2026-10-15)

**Goal**: The geometry and file-fidelity engines are single-source across **Android,
Windows, and iOS**. Never fork the mathematical CAD implementation into
platform-specific Java, Swift, and Windows copies.

This document defines a target architecture, **not a claim that three
shipping applications already exist**.

### Layer A — shared computational core

Path: `core/native/`

- **C11 source** built by CMake and accessed through a versioned, pure C API
  (`core/native/include/musacad_core.h`).
- First migrated actual production feature: **rational NURBS curve
  validation/evaluation/preview bounds**. Android's DWG scene now calls the
  same C ABI functions, not a separately maintained NURBS algorithm.
- Next migrated existing production feature: **DWG ACI/TrueColor, ByLayer,
  ByBlock and layer-0 INSERT color inheritance**. The Android compatibility
  include forwards to `core/native/include/musacad_color.h`. Its canonical
  color classification and resolution functions are compiled unchanged by
  Linux, Windows, and macOS C tests, plus the original Android native test.
  This is color evaluation, not yet a general multi-platform 2D renderer.
- The previous Android native include is now just a forwarding compatibility
  header for existing native regressions.
- Native DWG INSERT/MINSERT's 2D WCS affine composition now delegates to
  `core/native/include/musacad_affine.h` (identity, composition, mapping,
  checked inverse). The same C11 header is compiled by host tests; it is
  **not** a full 3D UCS kernel or multi-platform scene UI.
- Native DWG LWPOLYLINE/HATCH bulge-to-arc construction delegates to
  `core/native/include/musacad_bulge.h`. It keeps signed CW/CCW curvature,
  the existing Android OCS transform and safe line fallback for invalid
  arc geometry; shared unit tests cover both branches, translated coordinates,
  degenerate/NaN/overflow inputs, and source-safe failure.
- Native DWG ARC/CIRCLE/ELLIPSE and curved LWPOLYLINE scene preview now
  reuse `core/native/include/musacad_curve_bounds.h` for analytical
  positive/negative arc and transformed ellipse world-space extents.
  The prior full-ellipse bounds remain a conservative overflow fallback;
  geometry and first-paint color rendering are unchanged. Host C tests and
  iPhoneOS object-only compilation cover this shared math. This is not yet
  CAD viewport clipping or an editable 3D B-Rep kernel.
- A new independent portable 3D signed indexed-mesh mass kernel
  (`core/native/src/musacad_mesh_mass.c`) exposes volume and XYZ centroid
  with compensated origin-shifted sums. It mirrors the existing Java
  `Cad3dSignedVolume` calculations, uses the same unit-tetra/cube/WCS tests,
  and requires separately audited closed oriented topology. The existing
  Android Java implementation stays in place until the JNI integration and
  real-file equivalence tests are separately approved. This is NOT a
  complete ACIS 3DSOLID/B-Rep kernel or certified quantity takeoff.
- The iPhoneOS arm64 object-only CI step also syntax-compiles affine and
  bulge unit sources; it does **not** run them on a phone or constitute
  Windows/iOS app delivery.

- Core has no JNI, Android UI, Apple SDK, Win32, or LibreDWG headers. Native
  DWG I/O remains an **adapter** using pinned LibreDWG, with its own binary
  format support and licensing requirements.
- Future migrations, incrementally and with original DWG reference tests:
  2D geometry/edit operations; exact layer/entity semantics; native 3D mesh
  topology, signed volume, B-Rep where independently verified; file
  serialization and reference/undo contracts. The majority of today's Java
  CAD calculations **has not yet been migrated**.

### Layer B — independent platform adapters

| Platform | Adapter | Interaction and packaging |
|---|---|---|
| Android | Existing JNI + Java activity/UI; link `musacad_core` | Touch, gestures, storage access framework, APK/AAB **only after explicit final user approval** |
| Windows | C ABI adapter to a desktop UI (selection: Qt/C++ or C# + P/Invoke; choice still pending) | Mouse, keyboard, multi-window, Win32 file picker, EXE/MSIX |
| iOS | Swift/C bridge with C header and compiled arm64 native library | Touch, Files integration, Apple code signing/App Store requirements |

The adapters must not contain duplicate copies of CAD algorithms. They can
have platform-specific file dialogs, GPU/graphics backends, camera gestures,
license/store integration, fonts, export/printing and secure storage.

### Build and test evidence

`cmake -S core/native -B build/core -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug`
`cmake --build build/core --config Debug`
`ctest --test-dir build/core -C Debug --output-on-failure`

These commands build/test the shared C library alone, without any phone SDK
or DWG source checkout. The dedicated
`.github/workflows/cad-crossplatform-core.yml` targets Linux, Windows
and macOS host C compilers and **cross-compiles an iOS arm64 object** using
the installed Apple SDK. No iOS app, Windows GUI or physical device test is
implied by an object compilation pass. The Android native CMake target links
the same reusable library, and the existing Android and DWG quality gates
must pass before merging.

### DWG and licensing warnings

- Modern AutoCAD 2026 `AC1032` binary writing remains **unverified/not
  implemented**; the old R2000 host writer proof is not modern binary Save As.
- `EXTRUDE`, `REVOLVE`, and straight `SWEEP` polygonal groundwork is
  **not** a full ACIS 3DSOLID engine. Do not quote a fabricated parity score.
- Project/LibreDWG source is currently governed by GPL-3.0-or-later
  obligations. Multi-platform redistribution and commercial distribution
  require legal/licensing review (particularly iOS distribution terms).
  Reuse avoids rewriting code but does **not** automatically eliminate
  licensing, signing, maintenance, storefront or platform compliance costs.
- A user-approved APK or an iOS/Windows release cannot be built just
  because the shared library compiles. Do not disable existing test/release
  approval gates.

### Incremental acceptance

1. Single-source native NURBS implementation and public ABI **verified by
   cross-platform C tests and current Android/host DWG regression checks**.
2. Extract other CAD algorithms in small reviewed batches, using deterministic
   original-file regression fixtures for behavioral parity.
3. Implement Windows UI and Android adapter hardening independently.
4. Implement iOS Swift UI bridge; ensure iOS SDK, signing, architecture and
   redistribution licensing are accepted.
5. Independently validate real AutoCAD 2026 DWG files, edit/save/reopen
   fidelity, performance and physical devices for **each** platform.

The previous Android application must remain usable throughout this process.
