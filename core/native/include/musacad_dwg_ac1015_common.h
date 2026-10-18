/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_COMMON_H
#define MUSACAD_DWG_AC1015_COMMON_H
#include "musacad_dwg_ac1015_object.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Conservative R2000 subset of common ENTITY data, NOT general entities.
 * Unsupported EED/graphics or handle codes are rejected, never guessed.
 * This returns metadata and exact start bit for supported LINE/ARC/CIRCLE
 * field readers but does not independently establish drawing ownership,
 * layer reference validity, or full DWG semantic equivalence. */
typedef struct MusaDwgAc1015SimpleCommon {
    uint64_t indexed_handle;
    uint16_t type_code, color_index, invisibility;
    uint8_t nolinks, linetype_flags, plotstyle_flags, lineweight;
    double linetype_scale;
    size_t geometric_start_bit;
    int prefix_decoded;
} MusaDwgAc1015SimpleCommon;
int musa_dwg_ac1015_read_simple_common(
    const MusaDwgAc1015ObjectRecord *record,
    MusaDwgAc1015SimpleCommon *out);
#ifdef __cplusplus
}
#endif
#endif
