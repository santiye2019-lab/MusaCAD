# Native DWG engine

The experimental MusaCAD build links GNU LibreDWG under GPL-3.0-or-later.
This combined application is distributed under GPL-3.0-or-later (LICENSE).
AndroidX/Material and platform dependencies retain their respective licenses.

Pinned upstream source: https://github.com/LibreDWG/libredwg/tree/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43

Fetch that exact commit into third_party/libredwg before building. CI does this
with a separate checkout. No user drawing is included in the public repository.
Copy third_party/libredwg/COPYING to app/src/main/assets/COPYING-LibreDWG.txt.
Use Java 17, Android SDK 35, NDK 27.2.12479018 and CMake 3.22.1, then run
./gradlew assembleDebug. arm64-v8a and x86_64 are the configured architectures.
Native objects are linked with a 16 KiB maximum page size.

The bridge in app/src/main/cpp/dwg_bridge.c keeps a parsed DWG in an opaque
native session. app/src/main/cpp/native_scene.c walks model-space geometry,
including nested INSERT blocks, and emits a compact vector scene for fast first
paint and gesture navigation. The proven ASCII DXF conversion remains the
complete editable model and is prepared after the first native frame. The same
native code is built as a host executable; its smoke test requires both a
non-empty native scene and a successful DXF conversion from the pinned upstream
example_2013.dwg. Android JNI/device rendering still requires physical-device
testing.

CI publishes the application source, build scripts, license and pinned LibreDWG
source together as MusaCAD-source.tar.gz, alongside the host converter. Retain
and provide this corresponding-source bundle whenever distributing its APK.
Tap the MUSA CAD title to view the license and source repository link.

Runtime limits: native DWG parsing/conversion itself is not interruptible.
Cancellation discards the result after the native call returns. The compact
fast scene intentionally prioritizes common 2D primitives and block geometry;
the full editable DXF model remains the source of complete supported layers,
text, hatches, layouts, source edits and export. Original sharing always sends
the user's DWG.
