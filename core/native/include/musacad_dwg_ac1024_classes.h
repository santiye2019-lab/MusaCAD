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

#ifdef __cplusplus
}
#endif
#endif
