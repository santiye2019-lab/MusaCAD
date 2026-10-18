# Independent R2000 / AC1015 real binary section-locator reader

Portable C11 implementation: `core/native/src/musacad_dwg_ac1015.c`.
This is the first DWG **format-specific** read: it extracts real documented
R2000 section-locator records from offset `0x15` (count) and `0x19`
(record list). Every 9-byte entry holds record kind, little-endian offset and
length. It also captures the codepage and image seeker as **unverified**
metadata, verifies the 16-byte legacy file-header sentinel, and rejects
duplicate/unknown records, missing mandatory sections, overlapping nonempty
intervals, truncated headers and out-of-file ranges. The documented
3/4/5/6-locator variants are supported, including absent zero-length
optional sections (IDs 3..5); record 5 remains opaque metadata.

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
explicitly refused. Optional record ID 5 is preserved but **not decoded**.

Next stages: expand public real-file corpus and independent reference testing;
validate remaining section checksums and bounded section content decoding;
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

## Genuine six-locator acceptance result and honest limits

The pinned real `TS1.dwg` case has **six** locator records rather than
the prior synthetic fixtures' three to five. The source specification
defines a sixth-header XOR mask `0x8461`; the original document also
allows optional section records with no content. The pinned real file
contains a zero-length optional record (ID 3).

MusaCAD now validates all *non-empty* byte intervals for overlap and
out-of-file bounds, requires the three mandatory non-empty sections,
and preserves the original six-record metadata without borrowing bytes
from empty sections. Record 5 is accepted as opaque metadata but **not
decoded**. The pinned real-file CI gate independently checks the
six-record layout, empty optional record, safe refusal of its byte span,
HEADER and CLASSES CRC checks, original byte preservation, and explicit
modern-DWG rejection.

This is **one real reference file** proving only legacy header/section
read correctness within the tested scope. It does not establish
class/entity decoding, object maps, genuine modern DWG editing/writing,
complete DWG interoperability, or AutoCAD 2026 functional parity.

## P0: Native R2000 handle map index (object addresses, not decoded objects)

The **first-party portable C11** `musa_dwg_ac1015_handles_index` scans
AC1015 locator ID 2 (AcDb:Handles / OBJECT MAP) using the file-header
checks already accepted above. Each bounded map page has a big-endian
length (max 2032 bytes), alternating unsigned modular-char handle
deltas and signed modular-char absolute-file-location deltas, a
big-endian CRC-16 seeded with `0xC0C1`, and a required terminal
empty page. Handles must increase; every computed object address must
lie inside the **original** file. No partially filled caller index is
returned on a structurally invalid page, CRC failure, invalid delta,
missing terminal, or insufficient caller capacity.

The generic API supports an allocation-free count-only pass and
caller-owned entry storage. Public real `TS1.dwg` must pass the
new native-only index test on pinned original bytes. No LibreDWG
runtime or code enters the native engine; the external public fixture
is only reference data.

**LIMITS:** object map entries are original file locations and stable
handle IDs, not decoded entity types, curves, layers, layouts, solids,
or geometry. Further object-header/record checks and a multi-file
reference corpus are mandatory before claiming full DWG read/write.

## P0: First-party original object record CRC and type header

`musa_dwg_ac1015_read_object_record` accepts a bounded source address
and handle obtained from the previously checked R2000 object map. The
portable C11 reader validates the 2/4-byte unsigned DWG modular-short
record length, bit-short object type, bit-positioned raw 32-bit
pre-handle size and seeded `0xC0C1` CRC-16 over size bytes plus record
payload. It preserves output on error. The borrowed data is read-only.

The pinned public real `TS1.dwg` regression requires **all 336
indexed object records** to pass independently, not only the synthetic
fixture. It rejects truncated records, incorrect CRC, noncanonical
length, impossible bit boundaries, unsupported modern DWG and altered
source bytes. The external reference fixture is never modified.

**Scope:** type code and source boundaries are metadata only. They do
not yet decode actual LINE/CIRCLE/ARC fields, layer, color, object
handle-reference stream, geometry, B-Rep solids, XREF or user editing.
A valid CRC does not authenticate an adversarial file. AutoCAD 2026
equivalence remains independently unapproved.

## P0: shared AC1015 compressed coordinate field reader

`musacad_dwg_ac1015_values.c` is the first-party read-only **field**
decoder for R2000 binary objects (not a scene builder). It uses bounded
MSB-first bit access and little-endian raw-byte reconstruction for BS, BL,
RL, RD, BD, DD, BT and BE. All public reads are transactional: a malformed,
truncated, non-finite or disallowed field preserves both cursor and output.
A caller-provided exclusive bit limit prevents consuming the handle stream.

The already-CRC-verified object envelope now parses its raw BS type and RL
pre-handle bit limit through the same portable source. A CI acceptance probe
must continue checking every indexed record in the pinned real TS1.dwg;
new synthetic tests check all compressed-number modes, unaligned bit fields,
DD 4/6/8-byte deltas, default extrusion, and failed reads. The shared CMake
library is built on Android NDK, Windows, macOS and Linux; there is no
independent Java interpreter of these DWG fields.

**Not implemented here:** extraction of entity-common metadata/EED/graphic
streams, the authoritative start position of type-specific data, complete
LINE/CIRCLE/ARC entity payloads, handle reference graph, layer assignments,
or original edited DWG writing. `geometry_decoded` remains 0. Numeric
field decoding with synthetic coordinate streams is not equivalent to
real-DWG coordinate acceptance or AutoCAD 2026 feature parity.

Reference format: Open Design Alliance, *Open Design Specification for .dwg
files*, compressed data definitions and R2000 entity prescriptions,
https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf .

## P0: first-party R2000 LINE, CIRCLE and ARC field grammar

The common portable C11 core now contains strict original field-layout
parsers for AC1015 type 19 (LINE), type 18 (CIRCLE), and type 17 (ARC).
LINE fields: zero-Z flag; X RD/DD; Y RD/DD; optional Z RD/DD;
thickness BT and extrusion BE. CIRCLE: center 3BD, radius BD, BT/BE.
ARC: same CIRCLE fields, followed by start/end angles BD (radians).
Both ARC/CIRCLE reject non-positive radius; all three reject a zero
extrusion normal and a premature or overflowing pre-handle boundary.

The reader accepts only a CRC-verified object record and a **caller
supplied, independently validated first field bit**. Every exact
pre-handle bit must be consumed; any trailing mismatched bit, malformed
number or truncated record fails without partial output or source
changes. The fast CI uses synthetic bit-accurate real object envelopes
with CRC and source-record roundtrip; CMake registers the same regression
against Android-compatible portable static core. The full real pinned
`TS1.dwg` fixture still proves **record/CRC boundaries only**, not
full LINE/CIRCLE/ARC coordinate parity.

**Critical limitation:** reading the object's actual handle/EED,
optional graphic image and common entity metadata (color, layer
links, linetype scale and flags) to locate the authoritative first
entity-specific bit is **not yet implemented**. Current synthetic tests
provide an intentionally pre-validated offset. These parsers must
not be wired into rendering or used to declare any genuine native
DWG object semantically decoded until that trust boundary and
cross-reader real-file coordinate equality have passed.
`geometry_decoded` intentionally remains 0. Neither mobile APK nor
production service is changed. AutoCAD 2026 parity is unapproved.

Reference: Open Design Alliance Open Design Specification for .dwg files,
R2000 entity chapters LINE, CIRCLE and ARC,
https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf .

## P0: strict R2000 entity common prefix (safe subset)

`musa_dwg_ac1015_read_simple_common` is a first-party, read-only C11
decoder of R2000 object type/bit boundary, self-handle H, zero EED,
absent preview graphics, nolinks, ACI color, positive linetype scale,
linetype/plotstyle flags, invisibility and lineweight. It requires a
CRC-passed indexed AC1015 object and compares the encoded self-handle
against its object-map identity before producing a common-data
`geometric_start_bit`. It refuses EED payloads and preview graphics
rather than interpreting them as coordinates. Malformed/truncated
prefixes do not alter the caller's output; source bytes remain untouched.

The synthetic full-record fixture exercises this common-data boundary
through an existing LINE geometry field decoder, preserving original
CRC and the handle stream; fast CI and cross-platform CMake tests
compile the same portable code. **The real TS1.dwg regression still
only checks indexed record headers/CRC and must not be reported as
verified common metadata, layer references or real geometry.**

The core does not yet resolve actual owner/reactor/layer handle
references or object reachability, EED/preview forms, all entity
styles, independent real-file geometry values or modern binary DWG
writing. `geometry_decoded` must remain zero and the existing
Android renderer must not consume these fields as complete drawing
semantics until the real reference corpus passes.

## P0: R2000 first-party typed and relative handle field codec

`musacad_dwg_ac1015_handle_refs.c` now provides a portable, bounded,
transactional R2000 `H` value reader: 4-bit code, 4-bit byte count,
then big-endian handle/offset bytes (different byte order from RL/RD).
The recognized strict subset is primary absolute (0), typed absolute
soft/hard ownership and pointer references (2–5), and relative +1, -1,
+offset, -offset (6,8,A,C). All unsupported encodings, noncanonical
leading zero bytes, null target, malformed count, arithmetic underflow/
overflow and truncated streams are rejected without modifying the
caller cursor/output.

The same codec now validates each primitive object's self-handle
against the already CRC-verified object-map handle in the previous
common-prefix reader, instead of maintaining a second partial H codec.
A conservative `read_exact_handle_tail` API only parses the number of
handle fields already known **from an independently established entity
layout**, and rejects unexpected trailing data and nonzero tail padding;
its two-pass approach preserves all caller result slots on validation
failure. It deliberately does not assign handle roles.

A complete R2000 object graph is **not** yet implemented: the caller
must still determine the actual reference count and whether each handle
is owner/reactor/layer/linetype/plotstyle. References have not been
cross-checked against the referenced object's type or real original DWG
semantic golden records. Synthetic tests demonstrate handle bit layout,
arithmetic and immutable original CRC-backed record bytes, **not**
complete real-world entity graph parity. EED and embedded graphic payload
support remain separate. `geometry_decoded` stays zero for genuine
DWG object record readers. APK/release behavior is unchanged.

Specification: Open Design Alliance, *Open Design Specification for .dwg
files*, section "HANDLE REFERENCES" (code/count/relative handles),
https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf.
