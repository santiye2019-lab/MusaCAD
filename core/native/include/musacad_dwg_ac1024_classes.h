/* SPDX-License-Identifier: MIT
 * First-party R2010/AC1024 decompressed CLASSES structural prefix only.
 * NOT original section authentication, a class-name decoder, or a renderer.
 */
#ifndef MUSACAD_DWG_AC1024_CLASSES_H
#define MUSACAD_DWG_AC1024_CLASSES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Caller must explicitly choose the R2010 maintenance-dependent extra
 * RL variant. Never auto-detect from an untrusted byte sequence. */
enum {
    MUSA_AC1024_CLASSES_NO_EXTRA_RL = 0,
    MUSA_AC1024_CLASSES_EXTRA_RL = 1
};
typedef struct MusaDwgAc1024ClassesPrefix {
    uint32_t declared_data_area_bytes;
    uint32_t maximum_class_number;
    size_t data_start_byte;
    size_t records_start_bit; /* relative to start of declared data area */
    uint8_t extra_rl_present;
    uint8_t prefix_shape_verified;
    uint8_t original_section_authenticated; /* ALWAYS zero */
    uint8_t class_name_resolved;             /* ALWAYS zero */
    uint8_t display_accepted;                /* ALWAYS zero */
} MusaDwgAc1024ClassesPrefix;

/* Only accept an already decompressed, caller-provided complete logical
 * AcDb:Classes section byte span, not a .dwg file or an AcDbObjects span.
 * Checks opening/closing sentinels, bounds and the R2004+ numeric preamble
 * (BS maximum class; two RC=0; B=1). Deliberately does NOT authenticate
 * the section CRC, physical R2010 pages or individual class records.
 * A maximum >=505 cannot establish that class 505 is actually registered.
 * Output fail-atomic, source immutable, aliasing refused. */
int musa_dwg_ac1024_classes_prefix_shape_probe(
    const uint8_t *logical_classes, size_t logical_bytes,
    uint8_t extra_rl_present, MusaDwgAc1024ClassesPrefix *out);


/* Alternative R2010 R18+ documented header: RL data area, optional RL
 * maintenance word, MANDATORY RL bit count, then BS max class, RC0,
 * RC0, B1. This deliberately coexists with the older hypothesis and
 * does not promote any source bytes to a verified class/name/scene.
 * The supplied logical section is NOT independently authenticated here.
 * The extra-RL R2010 variant treats the declared area from byte +24:
 * it includes the four-byte bit count preceding the data at +28.
 * This matches the authenticated original AC1024 section-size witness.
 * The bitcount in that variant is also measured from the +24 RL start:
 * subtract exactly 32 bits before bounding the data stream at +28.
 * Neither the extra word at +20 nor the tail is assigned semantics.
 * An exact trailing 8-byte opaque footer is admitted only when the
 * closing sentinel itself sits at len-24; arbitrary trailers are refused.
 * Explicit layout choice avoids silently guessing maintenance rules. */
typedef struct MusaDwgAc1024ClassesR2010BitcountPrefix {
    uint32_t declared_data_area_bytes; /* R2010 extra-RL form INCLUDES bitcount RL */
    uint32_t declared_data_bits; /* source bitcount, extra-RL form INCLUDES 32-bit RL */
    uint32_t maintenance_word; /* opaque word at +20, NOT a validated semantic */
    uint16_t maximum_class_number;
    size_t bitstream_start_byte;
    size_t numeric_records_start_bit;
    uint8_t extra_rl_present;
    uint8_t end_sentinel_followed_by_eight_bytes;
    uint8_t byte_area_extent_exact;
    uint8_t structural_prefix_verified;
    uint8_t original_section_authenticated; /* ALWAYS zero */
    uint8_t class_name_resolved; /* ALWAYS zero */
    uint8_t display_accepted; /* ALWAYS zero */
} MusaDwgAc1024ClassesR2010BitcountPrefix;
int musa_dwg_ac1024_classes_r2010_bitcount_prefix_probe(
    const uint8_t *logical_classes, size_t logical_bytes,
    uint8_t extra_rl_present,
    MusaDwgAc1024ClassesR2010BitcountPrefix *out);

/* Pure read-only structural SURVEY of caller-supplied decompressed logical
 * Classes bytes. This diagnoses layout mismatches WITHOUT interpreting
 * individual class names or claiming source authentication. Even if all
 * flags pass, OT505 class identity and CAD rendering remain unverified.
 * Counts are not raw file bytes; all markers are shape observations.
 * Output must not overlap the source and is atomic on failure. */
typedef struct MusaDwgAc1024ClassesHeaderSurvey {
    uint8_t begin_at_zero;
    uint8_t end_at_eof;
    uint8_t begin_in_first64;
    uint8_t end_in_last64;
    uint8_t declared_byte_area_fits;
    uint8_t area_exact_0extra;
    uint8_t area_exact_1extra;
    uint8_t area_exact_2extra;
    uint8_t raw_bits20_in_range;
    uint8_t raw_bits24_in_range;
    uint8_t preamble_at20;
    uint8_t preamble_at24;
    uint8_t preamble_at28;
    /* Only last-64-byte sentinel uniqueness + positional arithmetic.
     * The three signed gaps are (end_sentinel_offset -
     * (candidate_data_offset + declared_data_bytes + 2 CRC bytes)).
     * Negative gaps are NOT silently rounded to zero. */
    uint8_t end_in_last64_unique;
    uint8_t end_trailing_bytes_known;
    uint8_t end_trailing_bytes; /* 0..48; no original DWG bytes */
    uint8_t declared_gap_known;
    int32_t declared_gap_from20;
    int32_t declared_gap_from24;
    int32_t declared_gap_from28;
    uint8_t original_section_authenticated; /* ALWAYS zero */
    uint8_t class_name_resolved;             /* ALWAYS zero */
    uint8_t display_accepted;                /* ALWAYS zero */
} MusaDwgAc1024ClassesHeaderSurvey;
int musa_dwg_ac1024_classes_header_survey(
    const uint8_t *logical_classes,size_t logical_bytes,
    MusaDwgAc1024ClassesHeaderSurvey *out);

/* R2010 class record numeric prefix, without external TU names, section
 * physical checksum, or class-body interpretation. Caller-supplied
 * decompressed logical section bytes are UNTRUSTED. */
typedef struct MusaDwgAc1024ClassNumeric {
    uint16_t class_number;
    uint16_t proxy_flags;
    uint16_t item_class_id; /* 0x1f2 entity; 0x1f3 object */
    uint16_t dwg_version;
    uint16_t maintenance_version;
    uint32_t object_instance_count;
    uint32_t unknown1;
    uint32_t unknown2;
    uint8_t was_proxy;
    uint8_t original_section_authenticated; /* ALWAYS zero */
    uint8_t class_name_resolved;             /* ALWAYS zero */
    uint8_t display_accepted;                /* ALWAYS zero */
} MusaDwgAc1024ClassNumeric;

/* Query count with records=NULL,capacity=0. All-or-nothing outputs, no
 * partial class records on failure. Unsorted/duplicate/missing IDs or
 * invalid object-vs-entity declarations are refused. This cannot establish
 * that OT505 is a material or allow it to paint/enter a scene. */
int musa_dwg_ac1024_classes_numeric_records(
    const uint8_t *logical_classes,size_t logical_bytes,
    uint8_t extra_rl_present,
    MusaDwgAc1024ClassNumeric *records,size_t capacity,size_t *out_count);

/* Numeric custom class records from a CRC-backed logical R2010 Classes
 * section supplied by the caller. This function does NOT authenticate
 * original physical DWG pages; only the file-level Classes extractor
 * can supply that provenance. It never reads TU names or permits paint.
 *
 * Requires a structurally exact R2010 RL bitcount prefix (0/8-byte
 * ending form), bounded source bits and ascending, unique class IDs.
 * Query with records=NULL,capacity=0. On any rejection, neither caller
 * records nor out_count is modified. The copied record flags always
 * carry zero original-source/name/display admission. */
int musa_dwg_ac1024_classes_r2010_numeric_records(
    const uint8_t *logical_classes,size_t logical_bytes,
    uint8_t extra_rl_present,
    MusaDwgAc1024ClassNumeric *records,size_t capacity,size_t *out_count);
/* The physical Classes section extraction APIs are deliberately NOT
 * exposed by this standalone module. Source page and CRC authentication
 * must be migrated and verified separately before they can be promised
 * to callers. */

/* Shape-only locator of the detached R2010 Unicode string bitstream.
 * Uses the final string-present bit of the declared class data and the
 * preceding 16/32-bit little-endian length fields. Does NOT read TU names,
 * check section CRC, link class numbers or authorize rendering.
 * All returned bit offsets are relative to bitstream_start_byte from the
 * R2010 prefix. On failure output is unchanged. */
typedef struct MusaDwgAc1024ClassesStringSpan {
    size_t stream_begin_bit;
    size_t stream_end_bit; /* exclusive; before trailing length words */
    uint32_t stream_bits;
    uint8_t wide_size_word;
    uint8_t bounded;
    uint8_t original_section_authenticated; /* ALWAYS zero */
    uint8_t class_name_resolved; /* ALWAYS zero */
    uint8_t display_accepted; /* ALWAYS zero */
} MusaDwgAc1024ClassesStringSpan;
int musa_dwg_ac1024_classes_r2010_string_span_probe(
    const uint8_t *logical_classes,size_t logical_bytes,
    uint8_t extra_rl_present,MusaDwgAc1024ClassesStringSpan *out);

/* R2010 class-name TU stream integrity audit. The numeric class order is
 * independently verified against the section's class records; each class
 * contributes 3 length-prefixed UTF-16LE fields in the detached stream.
 * Target metadata is a one-way diagnostic fingerprint, never a name or
 * permission to infer class semantics/paint. Caller authenticates the
 * original physical section separately. No raw name bytes leave the API.
 * Strict fail-atomic output with immutable input. */
typedef struct MusaDwgAc1024ClassesTuAudit {
    size_t classes_verified;
    size_t tu_fields_verified;
    size_t utf16_code_units_verified;
    size_t trailing_padding_bits; /* bounded 0..7 only */
    uint64_t target_triplet_fingerprint; /* non-cryptographic diagnostic */
    uint16_t target_class_number;
    uint16_t target_name_code_units[3];
    uint8_t target_found;
    uint8_t target_three_nonempty;
    uint8_t target_ascii_only;
    uint8_t complete_bounded_stream;
    uint8_t original_section_authenticated; /* ALWAYS zero */
    uint8_t class_semantics_accepted; /* ALWAYS zero */
    uint8_t display_accepted; /* ALWAYS zero */
} MusaDwgAc1024ClassesTuAudit;
int musa_dwg_ac1024_classes_r2010_tu_audit(
    const uint8_t *logical_classes,size_t logical_bytes,
    uint8_t extra_rl_present,uint16_t target_class_number,
    MusaDwgAc1024ClassesTuAudit *out);

/* Optional explicit lookup of ONE validated target class DXF identifier.
 * Never exports layer names, text entities, object handles, raw TU bytes or
 * other class names. Requires the FULL separate TU audit above and accepts
 * only 1..96 ASCII identifier characters [A-Za-z0-9_:.-] from the third TU
 * field. Fails atomically; raw source remains untouched. Not permission to
 * render, write, assign layer roles or infer class semantics. */
int musa_dwg_ac1024_classes_r2010_dxf_identifier(
    const uint8_t *logical_classes,size_t logical_bytes,
    uint8_t extra_rl_present,uint16_t target_class_number,
    char *output,size_t capacity,size_t *out_length);

#ifdef __cplusplus
}
#endif
#endif
