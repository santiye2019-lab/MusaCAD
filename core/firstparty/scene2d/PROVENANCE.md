# Independent 2D scene evaluator provenance (2026-10-28)

- **Scope:** `core/firstparty/scene2d/` of the original MusaCAD repository, **not** MusaCAD2.
- **Origin:** Independently authored C11 implementation from basic affine matrix algebra, binary search, graph traversal and generic CAD BYLAYER/BYBLOCK/Layer 0 behavior. No source code was imported from any existing MusaCAD, LibreDWG, third-party CAD project or other GPL/AGPL/LGPL library.
- **License:** MIT, see SPDX headers. Only standard C11 runtime and math library required.
- **Build boundary:** Separate CMake target and standalone tests. **Not linked to** `musacad_core` or the Android production DWG JNI. The existing core contains GPL-marked files and requires separate provenance/license review.
- **Capabilities:** Validates nested INSERT parent graphs, duplicate and dangling handles, layers, layer-0 inheritance, BYLAYER/BYBLOCK colors, inherited visibility, and composed 2D affine transforms. O((nodes+layers) log(nodes+layers)) iterative resolution; output rollback on failure.
- **Limitations:** Accepts already decoded nodes/layers, **not** AC1032 bytes. No viewport freeze, plot-style, ACI palette, XREF, dynamic block, font, or genuine DWG file support claimed. Production use requires integration and real DWG evidence.
- **Local verification:** `cmake -S core/firstparty/scene2d -B <build> -DBUILD_TESTING=ON && cmake --build <build> && ctest --test-dir <build> --output-on-failure`.
