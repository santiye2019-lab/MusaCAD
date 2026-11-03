# First-party verified-page Unicode text bridge (2026-11-03)

The new `musa_dwg1032_object_ms_utf16le_text` connects the existing
caller-verified RAW page directory, handle index, bounded MS candidate
payload window and strict UTF-16LE-to-UTF-8 converter. It reads only the
caller-supplied field range rather than assembling the entire object section.
The output is not published until the complete field is valid; invalid
surrogates, missing handles, malformed page boundaries and output aliasing
fail without exposing partial display text.

The caller MUST separately authenticate the AC1032 page map, page contents,
candidate object record, text entity type, payload field offset and UTF-16LE
byte length. MS framing is only an assumed candidate envelope. Neither this
bridge nor the synthetic 20,000-character and split-surrogate tests prove
that a real AC1032 TEXT/MTEXT entity has been decoded. Production Android
DWG import remains fail-closed. Authentic independently licensed fixtures
and end-to-end 2D color/layer/block verification are still required.

Implementation and tests were authored independently in C11 using only the
existing MIT first-party MusaCAD modules and generic bounded arithmetic.
No LibreDWG, GPL/AGPL/LGPL implementation, SDK source, or unknown-origin
code was copied, adapted or linked. `main`, `MusaCAD2/`, JNI production
import and existing 3D code are not modified.

## Ordinal viewport fast path (2026-11-03)

`musa_dwg1032_object_ms_utf16le_text_at_position` accepts a previously
validated object-index ordinal instead of performing another binary handle
lookup for each text field. It shares the same RAW-page/MS candidate and
strict UTF-16LE conversion boundaries as the handle-based API. The new
synthetic test checks cross-page surrogate parity, invalid ordinal rejection
and output-metadata alias rollback. This is a post-location optimization:
it does not recognize actual DWG TEXT/MTEXT object types, validate an
untrusted index from scratch, or enable production import.

The ordinal function and regression assertions were independently authored
for MusaCAD's MIT first-party C11 core, without copying or linking copyleft
or uncertain-provenance code. Existing 3D functionality, `MusaCAD2/` and
`main` remain outside this change.
