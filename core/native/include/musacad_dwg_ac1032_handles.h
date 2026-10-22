/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_HANDLES_H
#define MUSACAD_DWG_AC1032_HANDLES_H
#include "musacad_dwg_ac1018_handles.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original AC1032/R2018 AcDb:Handles page index into AcDb:AcDbObjects.
 * Reuse the single bounded, CRC-backed DWG handle-page codec already
 * implemented for AC1018; NEVER reinterpret modern object payloads as
 * legacy objects. Offsets are within the UNCOMPRESSED Objects section,
 * not physical DWG file offsets. Valid index != decoded object graph.
 * Immutable source; fail atomic for entries and out_count, reject aliases.
 * This pure C11 ABI works unchanged on Android, Windows and iOS. */
typedef MusaDwgAc1018HandleEntry MusaDwgAc1032HandleEntry;
int musa_dwg_ac1032_handles_index(
    const uint8_t *file, size_t file_bytes,
    MusaDwgAc1032HandleEntry *entries, size_t capacity,
    size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
