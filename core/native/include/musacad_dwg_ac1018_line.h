/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_LINE_H
#define MUSACAD_DWG_AC1018_LINE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only AC1018/R2004 LINE payload. Caller must supply a CRC-checked,
 * independently decompressed AcDbObjects section and a handle-map indexed
 * address. This function additionally verifies record CRC + self handle,
 * and refuses EED, embedded graphics and reactor-bearing records for now.
 * Does NOT establish layer/owner links or parity with independent CAD.
 */
typedef struct MusaDwgAc1018Line {
    uint64_t handle;
    uint32_t eed_raw_bytes_skipped; /* opaque EED metadata not decoded */
    double start[3];
    double end[3];
    double thickness;
    double extrusion[3];
    double linetype_scale;
    uint16_t color;
    uint16_t invisibility;
    uint8_t entity_mode;
    uint8_t lineweight;
    uint8_t linetype_flags;
    uint8_t plotstyle_flags;
    uint8_t xdictionary_missing;
} MusaDwgAc1018Line;
/* Atomic output: on false leaves *out untouched; never modifies source.
 * Input offset is relative to the uncompressed AcDbObjects section.
 */
int musa_dwg_ac1018_read_line_record(const uint8_t *object_section,
    size_t section_bytes, size_t object_offset, uint64_t indexed_handle,
    MusaDwgAc1018Line *out);
#ifdef __cplusplus
}
#endif
#endif
