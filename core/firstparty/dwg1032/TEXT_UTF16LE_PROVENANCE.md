# First-party Unicode text boundary (MIT)

The new `musa_dwg1032_text_utf16le` module and its regression tests were
authored for MusaCAD in standard C11. They do not copy, adapt, or link to
LibreDWG, ODA, GPL/AGPL/LGPL libraries, or an external CAD implementation.
The conversion uses elementary Unicode UTF-16 surrogate and UTF-8 scalar
encoding rules, without third-party source code.

**Verified boundary:** The caller supplies an already authenticated,
length-bounded UTF-16LE payload. The module checks malformed surrogates,
odd lengths, embedded NUL, buffer capacity and memory aliasing, and
converts to NUL-terminated UTF-8 without heap allocation. Failed input
validation leaves the public destination and length unchanged.

**Not implemented:** Locating AC1032 text objects, decoding DWG object
records, parsing compressed sections, DWG string encryption or version
flags, font resolution, SHX, MTEXT formatting, text placement, or JNI
rendering. Synthetic tests (including a 20,000-character buffer) are
not evidence of real DWG-file compatibility. Integration must wait for
a separately verified, independently authored record decoder.


## Bounded terminated-field adapter (2026-11-03)

The independent C11 `peek_cstr` and `take_cstr` functions extend the
previously authenticated cursor/UTF-16LE adapter. They scan only complete
two-byte UTF-16 code units, stop at the first aligned zero code unit, enforce
a caller-supplied maximum field size, and reject missing terminators,
malformed surrogate pairs, aliasing and insufficient UTF-8 capacity without
publishing partial output or advancing the cursor. The read-only preflight
reports exact UTF-8 allocation and source consumption sizes. A separate
20,000-code-unit regression exercises large-field bounds and late failures.

All code and synthetic tests in this extension were written specifically
for MusaCAD from elementary Unicode rules and existing first-party cursor
interfaces; no GPL, AGPL, LGPL or uncertain-origin CAD code was imported.

**Scope restriction:** These routines are safe adapters for a string whose
containing record and encoding have already been independently verified.
They do not discover DWG strings or establish authentic AC1032 text
compatibility. No JNI reader or 3D implementation is changed.
