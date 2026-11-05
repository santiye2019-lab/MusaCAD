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
 * Explicit layout choice avoids silently guessing maintenance rules. */
typedef struct MusaDwgAc1024ClassesR2010BitcountPrefix {
    uint32_t declared_data_area_bytes;
    uint32_t declared_data_bits;
    uint32_t maintenance_word; /* opaque, not a semantic assertion */
    uint16_t maximum_class_number;
    size_t bitstream_start_byte;
    size_t numeric_records_start_bit;
    uint8_t extra_rl_present;
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
/* First-party original AC1024 *physical* Classes section:
 * source signature, protected map and page CRC/Adler checked.
 * Only opaque logical bytes are exposed. No CLASS names, semantics,
 * drawing/rendering, file-open or import acceptance. */
int musa_dwg_ac1024_classes_section_probe(
    const uint8_t *file,size_t file_bytes,uint64_t *logical_bytes,
    size_t *page_count);
int musa_dwg_ac1024_extract_classes_section(
    const uint8_t *file,size_t file_bytes,uint8_t *output,
    size_t capacity,size_t *out_bytes);

#ifdef __cplusplus
}
#endif
#endif
