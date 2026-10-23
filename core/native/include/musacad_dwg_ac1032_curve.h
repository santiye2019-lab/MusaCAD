/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CURVE_H
#define MUSACAD_DWG_AC1032_CURVE_H
#include "musacad_dwg_ac1032_entity_prefix.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwgAc1032NativeCurve {
    MusaDwgAc1032EntityCommon common;
    uint64_t indexed_handle;
    uint64_t geometry_end_bit;
    double center[3];
    double radius;
    double thickness;
    double extrusion[3];
    double start_angle;
    double end_angle;
    uint16_t object_type; /* 17 ARC, 18 CIRCLE */
    uint8_t geometry_decoded;
} MusaDwgAc1032NativeCurve;
/* Field decoder from separately authenticated, source-derived common offset.
 * Does not invent a geometry offset or claim entire drawing availability. */
int musa_dwg_ac1032_curve_fields_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032NativeCurve *out);
/* Original complete AC1032 file -> authentic circle/arc object by indexed
 * handle, with verified physical sections, CRC and exact DATA/HANDLES
 * boundaries. Same first-party C11 ABI, fail-atomic and read-only. */
int musa_dwg_ac1032_read_curve_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032NativeCurve *out);
#ifdef __cplusplus
}
#endif
#endif
