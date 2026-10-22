/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_HANDLES_H
#define MUSACAD_DWG_AC1032_HANDLES_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_handles.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original R2018/AC1032 handle -> decompressed AcDbObjects offset index.
 * Calls the shared native bounded protected-section extractor AND the
 * independently authored 2004+ handle-page delta/CRC parser; it does NOT
 * reinterpret the pointed-to modern AcDb object as an R2004 record.
 *
 * The physical file must pass full AC1032 section catalog validation.
 * Output is ordered by increasing source handle; offsets refer ONLY to
 * the uncompressed AcDbObjects logical section (never file offsets).
 * count-only: entries=NULL,capacity=0. Fail-atomic and source immutable.
 * No LibreDWG/ODA runtime, no CAD binary writer or geometry claim.
 */
typedef MusaDwgAc1018HandleEntry MusaDwgAc1032HandleLocation;
int musa_dwg_ac1032_file_handle_index(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032HandleLocation *entries,size_t capacity,
    size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
