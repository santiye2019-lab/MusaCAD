/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_FILE_INDEX_H
#define MUSACAD_DWG_AC1018_FILE_INDEX_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_handles.h"
#ifdef __cplusplus
extern "C" {
#endif
/* From untouched raw AC1018 DWG file bytes, verify BOTH protected,
 * checksummed, compressed AcDbObjects and AcDb:Handles sections, then
 * construct the CRC-backed first-party handle -> object-section offset map.
 * This does NOT decode geometry, resolve entity ownership or write DWG.
 *
 * entries=NULL and capacity=0 is a validated count query.
 * Caller-owned buffers/count remain unchanged on failure. entries must not
 * overlap the original file; source file is never modified.
 * Max 1,000,000 indexed handles; read-only and platform independent.
 */
int musa_dwg_ac1018_file_handle_index(const uint8_t *file,
    size_t file_bytes,MusaDwgAc1018HandleEntry *entries,
    size_t capacity,size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
