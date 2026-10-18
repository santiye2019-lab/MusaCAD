/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_HANDLE_INDEX_H
#define MUSACAD_HANDLE_INDEX_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Portable first-party hash index, not a CAD SDK or a second object model.
 * Maps stable, nonzero 64-bit CAD handles to ordered document slot indices.
 * Caller owns handle uniqueness/reuse semantics and object allocations.
 * Hash index never retains CAD entity/geometry buffers.
 */
typedef struct MusaHandleIndex MusaHandleIndex;
MusaHandleIndex *musa_handle_index_create(size_t max_entries);
void musa_handle_index_destroy(MusaHandleIndex *idx);
size_t musa_handle_index_count(const MusaHandleIndex *idx);
/* 1 inserted, 0 duplicate, -1 invalid, -2 capacity reached. */
int musa_handle_index_put(MusaHandleIndex *idx,uint64_t handle,uint32_t slot);
/* 1 found, 0 missing, -1 invalid. On miss output untouched. */
int musa_handle_index_get(const MusaHandleIndex *idx,uint64_t handle,uint32_t *out);
/* 1 updated, 0 missing, -1 invalid. */
int musa_handle_index_set(MusaHandleIndex *idx,uint64_t handle,uint32_t slot);
/* 1 removed, 0 missing, -1 invalid. */
int musa_handle_index_remove(MusaHandleIndex *idx,uint64_t handle);
#ifdef __cplusplus
}
#endif
#endif
