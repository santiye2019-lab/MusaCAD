/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_REACTOR_LINKS_INTERNAL_H
#define MUSACAD_DWG_AC1018_REACTOR_LINKS_INTERNAL_H
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_dwg_ac1018_reactor_links.h"
/* INTERNAL contract: entries MUST be the immutable CRC-verified, sorted
 * AC1018 map owned by a successfully opened MusaDwgAc1018Scene.
 * Public record-level entry points retain full O(N) input validation.
 * No external callers may use this scene-only optimization with arbitrary
 * map memory. Output remains fail-closed and record CRC/roles/targets checked.
 */
int musa_dwg_ac1018_read_links_verified_scene_index(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,const MusaDwgAc1018HandleEntry *entries,
    size_t entry_count,uint64_t *reactors,size_t reactor_capacity,
    MusaDwgAc1018ReactorAudit *out,int expect_zero);
#endif
