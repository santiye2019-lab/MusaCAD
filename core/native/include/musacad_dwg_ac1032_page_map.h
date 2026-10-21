/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_PAGE_MAP_H
#define MUSACAD_DWG_AC1032_PAGE_MAP_H
#include "musacad_dwg_ac1018_page_map.h"
#ifdef __cplusplus
extern "C" {
#endif
/* First-party, read-only AC1032/R2018 PHYSICAL container admission.
 * Technical basis: publicly documented R2018 physical header/page-map
 * organization retains the R2013/R2004 page-map envelope. Reuse the same
 * independent C11 LZ+Adler/LCG decoder, without reinterpreting modern
 * R2018 object, string or handle payloads using legacy R2004 grammar.
 *
 * This is a syntax/physical bounds test only: not yet a verified R2018
 * AcDb:Handles object decoder, owner/XREF graph, geometry or Save As.
 * Caller original bytes unchanged; outputs untouched on failure.
 * Modern validation on independent native AC1032 files still required. */
typedef MusaDwgAc1018FileMapHeader MusaDwgAc1032FileMapHeader;
typedef MusaDwgAc1018PageLocation MusaDwgAc1032PageLocation;

int musa_dwg_ac1032_page_map_header(
    const uint8_t *file,size_t bytes,MusaDwgAc1032FileMapHeader *out);
int musa_dwg_ac1032_page_locations(
    const uint8_t *file,size_t bytes,MusaDwgAc1032PageLocation *locations,
    size_t capacity,size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
