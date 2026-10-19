/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_FILE_LINE_H
#define MUSACAD_DWG_AC1018_FILE_LINE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_line.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Portable, read-only, first-party AC1018 single-LINE ingestion:
 * protected raw DWG -> verified Objects & Handles sections -> CRC-backed
 * sorted handle index -> selected self-handle-checked LINE geometry.
 * A successful LINE is not a resolved owner/layer/reactor graph, nor
 * full CAD/DWG editing. Unsupported entity types fail closed.
 * On failure output and all source bytes are unchanged.
 */
int musa_dwg_ac1018_file_read_line(const uint8_t *file,size_t file_bytes,
    uint64_t handle,MusaDwgAc1018Line *out);
#ifdef __cplusplus
}
#endif
#endif
