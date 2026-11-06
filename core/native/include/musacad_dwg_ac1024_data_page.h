/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1024_DATA_PAGE_H
#define MUSACAD_DWG_AC1024_DATA_PAGE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_page_map.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original R2010 source data-page physical masked envelope/Adler proof.
 * The caller must independently authenticate original physical page map,
 * compressed section map and directory shape before this diagnostic.
 * No object streams, geometry, visibility, CAD scene or writing accepted. */
typedef struct MusaDwgAc1024DataPageEvidence {
    uint32_t verified_pages;
    uint64_t verified_packed_bytes;
} MusaDwgAc1024DataPageEvidence;
/* Verifies ALL directory page refs and both per-page Adler checksums.
 * No partial evidence output or source writes on ANY refusal. */
int musa_dwg_ac1024_data_pages_authenticated(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024DataPageEvidence *out);
#ifdef __cplusplus
}
#endif
#endif
