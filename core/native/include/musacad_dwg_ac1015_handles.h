/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_HANDLES_H
#define MUSACAD_DWG_AC1015_HANDLES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* AC1015/R2000 object-map index entry. File offset is the ABSOLUTE location
 * of the original opaque object record; its bytes are NOT decoded here. */
typedef struct MusaDwgAc1015HandleEntry {
    uint64_t handle;
    uint64_t file_offset;
} MusaDwgAc1015HandleEntry;
/* First-party read-only, bounded R13-R15 object-map decoder.
 *
 * Page lengths and CRC are big-endian; handle and signed location deltas
 * are DWG modular characters. Require every page CRC, explicit terminal
 * page, strictly increasing handles and in-file object positions.
 *
 * Pass entries=NULL and capacity=0 to COUNT validated entries only.
 * For indexed output supply capacity>=number of entries.
 * On any failure preserve caller's *out_count and entries unchanged,
 * including insufficient capacity. Input file MUST remain immutable for
 * the entire call. Never treat this index as decoded object geometry.
 */
int musa_dwg_ac1015_handles_index(const uint8_t *file,size_t file_bytes,
                                  MusaDwgAc1015HandleEntry *entries,
                                  size_t capacity,size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
