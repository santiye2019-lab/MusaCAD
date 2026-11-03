# Bounded counted UTF-16LE field adapter (2026-11-02)

- **Scope:** Original MusaCAD, `core/firstparty/scene2d/` only. No changes to `MusaCAD2/`, `main`, existing 3D modules or production Android JNI.
- **Origin:** Independently authored standard C11 wrapper around the already first-party strict UTF-16LE scalar decoder. Based on basic byte-count arithmetic and Unicode scalar validity, not on any third-party DWG reader, copyleft source or undocumented file format implementation.
- **License and dependency boundary:** MIT SPDX sources, standard C11 only; no GPL/AGPL/LGPL source copied, adapted, or linked.
- **Purpose:** Given a caller-verified UTF-16 *code-unit* count and bounded record, validate and decode exactly that many code units into UTF-8, including surrogate pairs, while returning bytes consumed. Trailing record bytes remain uninterpreted; an odd trailing byte is permitted. Empty fields, size queries and bounded outputs are supported.
- **Safety:** Reject count outside record bounds, oversized records, invalid Unicode, overlapping input/output/count storage, missing output terminator capacity and malformed pointers. Failed calls preserve both output and counts. Large 20,000-code-unit synthetic tests exercise bounded conversion.
- **Limitation:** No AC1032 section decompression, text-field discovery, length-prefix interpretation, TEXT/MTEXT schema, SHX/TTF shaping, rendering or production integration. This API must receive verified framing from a future independent AC1032 decoder. It is not evidence of complete DWG text support.
- **Validation:** `cmake -S core/firstparty/scene2d -B build-scene2d -DBUILD_TESTING=ON && cmake --build build-scene2d && ctest --test-dir build-scene2d --output-on-failure`. Verify GitHub CI results independently.
