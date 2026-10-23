/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_LINE_FILE_H
#define MUSACAD_DWG_AC1032_LINE_FILE_H
#include "musacad_dwg_ac1032_entity_prefix.h"
#include "musacad_dwg_ac1032_line_fields.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Single native first-party AC1032 object-to-LINE API.
 * Reads the immutable full original DWG, authenticates physical pages,
 * handle-index/record CRC and DATA/HANDLES partition, then derives the
 * true R2018 common-entity geometry offset without scanning.
 * Strict simple LINE subset; returns failure for unsupported fields.
 * Does NOT claim a complete drawing, scene ownership or DWG writer.
 */
typedef struct MusaDwgAc1032NativeLine {
    MusaDwgAc1032LineCommon common;
    MusaDwgAc1032LineFields geometry;
} MusaDwgAc1032NativeLine;
/* Fail-atomic: out unchanged on ANY failure; never modifies file bytes.
 * Rejects aliased buffers. No third-party CAD decoder/runtime.
 * Same portable C11 ABI used on Android, Windows and iOS.
 */
int musa_dwg_ac1032_read_line_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032NativeLine *out);
#ifdef __cplusplus
}
#endif
#endif
