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
- A new **opt-in Android JNI adapter** (`mesh_mass_jni.c` and
  `Cad3dNativeMeshMass.java`) connects the existing checked triangle mesh
  to the same portable 3D mass C ABI. It compares the native volume/XYZ
  centroid with the existing Java `Cad3dSignedVolume` result and exposes a
  clear diagnostic (pass, mismatch, native unavailable, rejected). Existing
  Java/UI CAD volume behavior remains authoritative and unchanged until
  real-device accuracy/performance tests allow switching. Host JVM tests
  prove no native library is required for fallback. Android's native bridge
  is **not** a Windows/iOS application or ACIS-solid implementation.
- A portable 3D triangle **surface-area and area-weighted centroid** engine
  (`musa_core_mesh_surface_f32`) now mirrors the existing read-only Android
  Java `Cad3dSurfaceMetrics` math, including compensated sums and WCS origin
  shifts. The common C core includes open surface, cube, large WCS, zero-area
  and malformed mesh regression tests. It is compiled on Linux/Windows/macOS,
  plus iPhoneOS arm64 object-only. Android's existing Java output is unchanged
  until an independently tested adapter is selected. This is NOT analytic
  ACIS face area or certified engineering metraj.
- Android-only optional surface JNI bridge `mesh_surface_jni.c` and
  `Cad3dNativeMeshSurface.java` now compare shared C11 area/area-weighted
  centroid/face counts against unchanged Java `Cad3dSurfaceMetrics`.
  Open/degenerate meshes, NaN centroids, large world coordinates and missing
  JNI are guarded by host JVM/host JNI compilation tests. This opt-in
  bridge does not replace existing Android result, execute on the drawing UI
  thread or certify an exact B-Rep surface.
- Portable 3D indexed-mesh **topology preflight** now mirrors the Android
  `Cad3dMeshTopology` screening in `musacad_mesh_topology.c`. It reports
  open edges, inconsistent winding, non-manifold edges, degenerate faces and
  duplicate/zero-thickness opposed facets. It rejects invalid indices and
  nonfinite coordinates; meshes exceeding 50,000 triangles remain explicitly
  unaudited. The same C ABI can gate `musa_core_mesh_mass_f32` on all
  platforms. Separate real-file cross-tests and geometric self-intersection
  validation are still required before certified solid/metraj claims.
- A new cross-platform `core/native/include/musacad_affine3.h`
  provides common **3D WCS affine composition, point/vector mapping,
  mirrored/nonuniform scale handling, inverse transform and orientation
  determinant**. These are directly tested on Linux/Windows/macOS and
  syntax-compiled for iPhoneOS arm64. This is reusable 3D BLOCK/UCS math
  groundwork; the current Android 3D rendering/editing implementation
  is not replaced, and AutoCAD 2026 3D UCS command parity is NOT claimed.
- A read-only native 3D **ray-versus-triangle picking** API
  `musa_core_mesh_ray_pick_f32` now locates the nearest triangle and
  returns drawing-unit distance, WCS hit point, barycentric parameters,
  face index and winding orientation. It is common C11 math with no
  Android/Win32/Swift dependencies. Unit tests cover nearest-face ordering,
  backfaces, large WCS offsets, degenerate triangles and invalid geometry,
  and CI compiles the same source on Linux, Windows, macOS and iPhoneOS
  arm64. Scene acceleration/BVH, rendering, physical-device hit targets,
  ACIS/B-Rep and complete object selection are future work.
- The `musacad_bounds3.h` pure-C **conservative AABB broad phase**
  now rejects many obviously out-of-ray triangles before detailed picking
  in `musacad_mesh_ray_pick.c`. Float-WCS tolerance deliberately favors
  false positives rather than missed geometry, and later bad triangles are
  still independently validated. Separate regression tests exercise large
  drawing coordinates, zero-thickness bounds, edge hits and invalid inputs.
  This is a per-face shortcut: computational complexity remains O(faces);
  full BVH acceleration, phone performance acceptance and GPU-based picking
  are future work.
- Deterministic BVH acceleration counters
  (`musa_core_mesh_bvh_pick_stats`) now report visited nodes, rejected
  nodes and exact triangle intersection counts without timing noise.
  Shared tests check fewer than 96 exact triangle checks on dispersed
  96-face CAD test geometry. This is evidence of algorithmic pruning,
  not yet a measured FPS or real-file latency guarantee.
- New `musacad_mesh_bvh.c` implements an **actual spatial BVH**
  (hierarchical per-face 3D AABB partitions) for repeated closest-face
  ray picks. Its index owns a validated copy of mesh vertices/faces,
  supports WCS, and compares exact hits against the existing shared
  ray/triangle implementation. Balanced recursive nodes avoid scanning
  all triangles on typical dispersed engineering projects; extremely
  overlapping geometry remains worst-case linear. This is a reusable
  native C11 geometry facility for Android/Windows/iOS, not an Android
  3D UI integration, guaranteed performance claim, editable solids, or
  AutoCAD 2026 equivalence certification.
- Object-local BVH instances can now be selected from real drawing
  **WCS rays with nested INSERT transforms** via `musacad_bvh_wcs.h`.
  The same BVH index is reused under mirror, translation, rotation,
  shear and nonuniform scale; WCS distances and hit coordinates are
  returned without confusing object-space units. Host cross-platform
  regression tests cover identity, odd reflections, 90-degree rotation,
  1,000,000+ unit translated drawings, invalid/singular transforms and
  result preservation. A common C11 math capability, NOT a shipped
  Android hit-selection UI or proof of AutoCAD 2026 object parity.
- Read-only multi-object **3D visible scene picking** now lives in
  `musacad_bvh_scene.h` and `musacad_mesh_bvh_scene.c`. One
  indexed mesh may be reused across independent BLOCK/INSERT instances,
  each carrying its own WCS affine transform, source object ID and
  externally resolved visibility. The nearest visible WCS hit is
  deterministic; invalid visible geometry fails without publishing any
  earlier hit. Diagnostic counters collect real BVH node and face checks.
  Full DWG layer/xref resolution, UI object-selection integration and
  platform performance acceptance remain outside this primitive.
- Multi-instance **WCS AABB coarse rejection** now avoids traversing the
  indexed mesh BVH for 3D objects that cannot meet the selection ray.
  `musa_core_mesh_bvh_bounds` exposes the immutable local root box;
  eight affine-transformed corners yield a conservative world-space
  box under translation, rotation, mirror, shear and nonuniform scaling.
  Validity and invertibility of every visible instance is checked before
  coarse rejection, so corrupt off-screen objects cannot be silently
  accepted. Diagnostic `bounds_rejected_instances` counters are
  cross-platform tested using 32 far-away instances, nearest WCS hit,
  mirrored shear and fail-closed singular transforms. It is still a
  linear walk over scene instances; a separate scene-level BVH and
  real-DWG phone benchmarks are future work.
- Shared **3D camera screen-to-WCS picking rays** are now computed by
  `musacad_camera_ray.c`, a portable C11 view-projection 4x4 matrix
  unprojection routine. It accepts top-left viewport pixel-edge coordinates,
  both OpenGL-style -1..1 and Metal/D3D-style 0..1 clip depth, orthographic
  and perspective cameras, and world drawing coordinates as doubles.
  Its unit tests check singular/NaN matrix safe rejection, far-from-origin
  WCS coordinates, screen-corner ray direction, near/far perspective rays,
  and direct consumption by the multi-BLOCK 3D BVH scene selector.
  No Android gesture integration, iOS viewport implementation or true
  real-DWG multi-device parity has been claimed.
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
