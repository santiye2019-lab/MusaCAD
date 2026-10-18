# Independent R2000 / AC1015 real binary section-locator reader

Portable C11 implementation: `core/native/src/musacad_dwg_ac1015.c`.
This is the first DWG **format-specific** read: it extracts real documented
R2000 section-locator records from offset `0x15` (count) and `0x19`
(record list). Every 9-byte entry holds record kind, little-endian offset and
length. It also captures the codepage and image seeker as **unverified**
metadata, verifies the 16-byte legacy file-header sentinel, and rejects
duplicate/unknown records, missing mandatory sections, overlapping intervals,
truncated headers and out-of-file ranges.

**Reference**: Open Design Alliance, *Open Design Specification for .dwg
files*, chapter 3.2 (R13-R15 / R2000 file header), especially section-locator
records and post-header sentinel:
https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf

This is an independently written **first-party** decoder of the publicly
described layout, NOT translated LibreDWG/ODA source code. All Android,
Windows, iOS share the exact C implementation. CTests and CI include synthetic
byte fixtures, overflows, corrupt sentinel, unknown/duplicate classes,
truncations and byte-preservation assertions.

**CRITICAL LIMITATIONS**: extracted offsets are structurally checked and
the documented R2000 header CRC is now verified using the section-count
XOR mask. Section contents and their own integrity are not decoded. No B-Rep, handles, objects, 3D, drawing edit, modern AC1032 layout
or DWG saving is supported by this specific parser. Its output must NOT
admit an editable document or claim full DWG compatibility. AC1018+ is
explicitly refused. Section record number 5 is undocumented and refused.

Next stages: verify real-file fixtures independently; validate per-section
content sentinels/checksums; bounded section content decoding;
then implement modern AC1032 separate section map/compression protocols.
MusaCAD's MC1032 `.mcad` is a different, custom format.

## P0 read-only section view (all three platforms)

`musa_dwg_ac1015_readonly_section` returns a borrowed, bounded original
byte span for a documented AC1015 section kind (0..4), if and only if the
header CRC, sentinel, complete section table and interval checks **all**
succeed on this invocation. It refuses absent/unknown section kinds,
truncated/mutated header, stale validation and null outputs without changing
caller outputs or modifying the input. The C11 function is part of the single
Android/Windows/iOS common native core; no LibreDWG runtime dependency.

**Do not confuse safe access with decoded DWG**: changes to a section's
payload are not detected by the file-header CRC. The returned pointer is
borrowed and invalidated if its owning allocation goes away. Neither
section contents nor object maps/handles/geometry are authenticated, decoded
or made editable. The CI regression explicitly tests this limitation.

## P0: R2000 framed HEADER and CLASSES reading

`core/native/src/musacad_dwg_ac1015_frame.c` adds one original,
platform-independent, read-only C11 routine for legacy R2000/AC1015
**locator 0 (HEADER variables)** and **locator 1 (CLASSES)**. It
requires the previously checked original file-header CRC/section table,
then checks the documented 16-byte beginning sentinel, matching
complement ending sentinel, 4-byte little-endian payload length and exact
section extent. Borrowed payload is exposed only if all checks pass;
invalid length, changed sentinel, missing or unsupported section, newer
DWG version and malformed header fail without changing caller output.
Portable CTest and GitHub fast regression cover synthetic positive and
negative cases.

**Hard boundary:** the stored payload CRC is *reported but not validated*.
Corruption **inside** the payload can still pass framing checks, as the
negative-control test explicitly demonstrates. Neither HEADER variables
nor CLASSES records are decoded. This provides safe framing, **not**
full content integrity, general-purpose DWG opening/writing, real-file
conformance, or AutoCAD 2026 equivalence.

Format fields and sentinels are independently transcribed from
[Open Design Specification for DWG files](https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf),
R13-R15 section locators, AcDb:Header and AcDb:Classes descriptions.
Production on-file fixtures and verified payload CRC are separate next
acceptance gates.

## P0: Verified HEADER / CLASSES data CRC (strict optional API)

`musa_dwg_ac1015_parse_crc_verified_frame` extends, without changing,
the existing envelope-only API. After validating the original R2000
file header, section table and both sentinels, the strict function checks
the **stored section CRC-16** using initial seed `0xC0C1`, applied
to the 4-byte little-endian section length followed by the entire payload.
Any mismatch fails closed and leaves the caller's output unchanged.
Its returned `payload_crc_verified=1` distinguishes a checked span from
the older `parse_frame` function's always-unverified CRC.

The seed and covered bytes are specified in the ODA *Open Design
Specification for .dwg files*, section 9 (AcDb:Header) and section 10
(AcDb:Classes). Tests use fixed independent golden CRC vectors,
deliberately corrupted section payloads, a corrupted stored CRC, invalid
arguments and an intentionally re-signed payload to demonstrate that
CRC-16 detects accidental corruption but provides **no authenticity**.

**Acceptance scope:** synthetic R2000 envelopes and portable host tests;
the file remains read-only. Real DWG fixture corpus and independent
third-party decoder equivalence, object/class decoding, complete section
validation, modern AC1032 decoding and native DWG writing remain open.

## P0: Real pinned R2000 binary fixture acceptance gate

`core/native/tests/core_ac1015_real_fixture_probe.c` is an original,
read-only host test that calls the same portable C functions as MusaCAD.
The Modern DWG CI workflow checks out the publicly distributable
LibreDWG test-data repository at an **exact pinned source commit**,
then runs the **MusaCAD-owned reader only** on the original
`test/test-data/2000/TS1.dwg`. The expected on-disk signature is
`AC1015`. We require the file-level CRC/locators and both HEADER and
CLASSES envelope/data CRCs to pass. The original file SHA-256 before and
after execution must match exactly.

A newer sample (`test/test-data/example_2013.dwg`) is also required to
be rejected by this AC1015-only parser. CI must **fail** on any
rejection of the R2000 file or mistaken acceptance of the modern file:
no soft-pass, warning downgrade, or skipped real acceptance. The
first-party probe does not load or call any LibreDWG library.

This genuine **file-format section-level** evidence remains distinct
from entity/class/object decoding, editable scene reconstruction,
object-preserving DWG save, AutoCAD 2026 equivalence and physical
Android/Windows/iOS application testing. Never assert any of those
broader claims based only on this gate.
