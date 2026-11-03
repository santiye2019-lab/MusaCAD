/* SPDX-License-Identifier: MIT
 * First-party R2010/AC1024 read-only container page-map gate.
 * R2010 shares the R2004 header and physical page-map envelope, but NOT
 * R2004 entity bit coding. Only verified page locations are reported.
 * No geometry, layer, color, ownership, rendering, or write admission.
 */
#ifndef MUSACAD_DWG_AC1024_PAGE_MAP_H
#define MUSACAD_DWG_AC1024_PAGE_MAP_H
#include "musacad_dwg_ac1018_page_map.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef MusaDwgAc1018FileMapHeader MusaDwgAc1024FileMapHeader;
typedef MusaDwgAc1018PageLocation MusaDwgAc1024PageLocation;
/* Fail closed: on any failure do not modify caller output. */
int musa_dwg_ac1024_page_map_header(
    const uint8_t *file,size_t file_size,MusaDwgAc1024FileMapHeader *out);
int musa_dwg_ac1024_page_locations(
    const uint8_t *file,size_t file_size,MusaDwgAc1024PageLocation *entries,
    size_t capacity,size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
