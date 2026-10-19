/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_CURVE_H
#define MUSACAD_DWG_AC1018_CURVE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only R2004 circle/arc geometry from separately verified and
 * decompressed AcDb:AcDbObjects section. No section decompression or
 * ownership/layer graph performed here. Unsupported entity extras rejected. */
typedef struct MusaDwgAc1018Curve {
    uint64_t handle;
    uint16_t type_code; /* 17 ARC, 18 CIRCLE */
    double center[3];
    double radius;
    double thickness;
    double extrusion[3];
    double start_angle;
    double end_angle;
    double linetype_scale;
    uint16_t color;
    uint16_t invisibility;
    uint8_t entity_mode;
    uint8_t lineweight;
    uint8_t linetype_flags;
    uint8_t plotstyle_flags;
    uint8_t xdictionary_missing;
} MusaDwgAc1018Curve;
/* Atomic on failure, never mutates source. Requires trusted handle map entry.
 * Does not assert independent commercial CAD compatibility. */
int musa_dwg_ac1018_read_curve_record(const uint8_t *object_section,
    size_t section_bytes, size_t object_offset, uint64_t indexed_handle,
    MusaDwgAc1018Curve *out);
#ifdef __cplusplus
}
#endif
#endif
