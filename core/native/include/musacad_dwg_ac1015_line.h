/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_LINE_H
#define MUSACAD_DWG_AC1015_LINE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1015_object.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only LINE-specific R2000 fields only.
 *
 * The caller MUST have independently validated the exact first bit AFTER
 * common entity data, including EED, optional preview and color metadata.
 * This function cannot derive or authenticate that offset and therefore
 * MUST NOT be used as proof that original DWG geometry was accepted.
 * The record must come from the CRC-checked original object map parser.
 */
typedef struct MusaDwgAc1015LineFields {
    uint64_t indexed_handle;
    double start[3];
    double end[3];
    double thickness;
    double extrusion[3];
    size_t first_field_bit;
    size_t end_field_bit;
    int field_layout_decoded; /* 1 only for matching field-boundary test */
} MusaDwgAc1015LineFields;

int musa_dwg_ac1015_decode_line_fields(
    const MusaDwgAc1015ObjectRecord *record,
    size_t verified_common_data_end_bit,
    MusaDwgAc1015LineFields *out);
#ifdef __cplusplus
}
#endif
#endif
