# AC1032 fixed preamble: authentic-source regression — 2026-11-03

Scope: MusaCAD first-party branch PR #705; no production Android admission, file rewrite, or DWG model-parity claim.

## Confirmed failure

Two independently supplied, authentic AC1032 drawings (7,410,775 and 7,280,163 bytes) share modern fixed-preamble metadata with **nonzero bytes between 0x2C and 0x43**. The pre-existing `musa_dwg1032_read_preamble_prefix` incorrectly enforced zero for **all bytes 0x2C..0x7F**, so both drawings were falsely rejected before section/page authentication.

This was a **version-specific validation error**, not proof that geometry decoding works. The older R2004 metadata layout has a distinct reserved range; R2007+ includes an Application Info offset at 0x2C, and AC1032 real files can carry additional writer metadata. See the Open Design Alliance DWG format specification, R2007 metadata description and its R2018 format revisions; only the field-location facts were used, no external decoder implementation.

## Independent fix

- Preserve exact AC1032 magic, file length, reserved 0x06..0x0A and 0x15..0x17, required 0x80 pointer, and all existing preview/summary/VBA bounds checks.
- Validate the AppInfo offset at 0x2C (zero is permitted, nonzero must be in the file after the first 0x100 bytes).
- Treat 0x30..0x43 as opaque, unverified writer metadata rather than demanding zeros. Keep 0x44..0x7F zero-guarded.
- Add C11 regression with the observed modern metadata bytes and the two actual file lengths, plus negative tests for malformed AppInfo addresses, out-of-file values, and nonzero reserved bytes. No proprietary object content or original binary drawing is committed.

## Honest acceptance status

Passing preamble parsing proves **only** bounded fixed-prefix recognition. It does not verify encrypted/RS-protected header pages, section maps, object maps, handles, layers, blocks, UTF-16, coordinates, geometry, colors, visibility, 3D, or write/reopen parity. The C11 regression is derived from real source metadata but is **not** an end-to-end test on the private DWG bytes. Actual drawings remain outside this repository and must be rerun through the existing read-only `tests/private_dwg_read_audit.py` and, eventually, an independent golden geometry baseline.

Production JNI stays fail-closed; no APK or release approval is implied.
