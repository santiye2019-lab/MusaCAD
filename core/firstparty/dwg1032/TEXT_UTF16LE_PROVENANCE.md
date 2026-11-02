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
