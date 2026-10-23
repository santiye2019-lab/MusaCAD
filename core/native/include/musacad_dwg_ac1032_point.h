/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_POINT_H
#define MUSACAD_DWG_AC1032_POINT_H
#include "musacad_dwg_ac1032_entity_prefix.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native first-party R2018 POINT (fixed object OT=27): X/Y/Z 3BD,
 * BT thickness, BE extrusion and BD UCS X-axis angle.
 * Bounded immutable DATA, not guessed floats or an Android-only parser.
 */
typedef struct MusaDwgAc1032NativePoint {
    MusaDwgAc1032EntityCommon common;
    uint64_t indexed_handle;
    uint64_t geometry_end_bit;
    double point[3];
    double thickness;
    double extrusion[3];
    double x_axis_angle;
    uint8_t geometry_decoded;
} MusaDwgAc1032NativePoint;
int musa_dwg_ac1032_point_fields_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032NativePoint *out);
int musa_dwg_ac1032_read_point_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032NativePoint *out);
#ifdef __cplusplus
}
#endif
#endif
