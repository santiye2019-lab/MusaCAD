/* SPDX-License-Identifier: MIT
 * First-party index for caller-decoded object locations. This does NOT
 * decode AC1032 object maps, compressed pages, or entity records.
 */
#ifndef MUSA_DWG1032_OBJECT_INDEX_H
#define MUSA_DWG1032_OBJECT_INDEX_H
#include <stddef.h>
#include <stdint.h>
#include "musa_dwg1032_handle.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwg1032ObjectLocation {
    uint64_t handle;
    uint64_t offset;
    uint64_t length;
} MusaDwg1032ObjectLocation;
typedef struct MusaDwg1032ObjectIndex {
    const MusaDwg1032ObjectLocation *entries;
    size_t count;
    uint64_t section_bytes;
} MusaDwg1032ObjectIndex;
/* Caller supplies immutable entries ordered by strictly increasing, nonzero
 * handles. Each nonempty [offset,offset+length) lies in the verified section.
 * Does NOT prove ranges are disjoint. Failure leaves index unchanged. */
int musa_dwg1032_object_index_init(MusaDwg1032ObjectIndex *index,
                                    const MusaDwg1032ObjectLocation *entries,
                                    size_t count, uint64_t section_bytes);
/* O(log count). Requires a validated, unmodified index; output unchanged
 * on failure. Entries must outlive the index. */
int musa_dwg1032_object_index_find(const MusaDwg1032ObjectIndex *index,
                                    uint64_t handle, size_t *position);
/* Resolves a decoded reference then looks up its target. Does not establish
 * semantic ownership, object type, or layer membership. */
int musa_dwg1032_object_index_find_ref(const MusaDwg1032ObjectIndex *index,
                                        const MusaDwg1032Handle *reference,
                                        uint64_t base_handle,
                                        size_t *position);
#ifdef __cplusplus
}
#endif
#endif
