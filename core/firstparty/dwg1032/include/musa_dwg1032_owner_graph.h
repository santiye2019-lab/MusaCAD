/* SPDX-License-Identifier: MIT
 * Independent post-decode ownership graph validation and root resolution.
 */
#ifndef MUSA_DWG1032_OWNER_GRAPH_H
#define MUSA_DWG1032_OWNER_GRAPH_H
#include "musa_dwg1032_object_index.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Caller-owned temporary entry. Contents are unspecified after a failure. */
typedef struct MusaDwg1032OwnerWork {
    uint64_t root;
    size_t depth;
    size_t previous;
    uint8_t state;
} MusaDwg1032OwnerWork;
/* Resolve the root and hop count of EVERY indexed, predecoded owner chain.
 * owner_handles[i] == 0 marks a root. index must have been validated by
 * object_index_init and must remain immutable. All buffers must be disjoint.
 * work_count must be >= index->count, and the ENTIRE advertised work
 * capacity must be disjoint from the input and output buffers, including
 * any unused tail. No recursion or heap allocation.
 * O(n log n) for n objects, with O(n) caller-provided workspace.
 * Rejects dangling owners and cycles. On failure roots and depths remain
 * unchanged; work is scratch and may be modified. No DWG bytes are parsed. */
int musa_dwg1032_owner_graph_resolve(const MusaDwg1032ObjectIndex *index,
                                     const uint64_t *owner_handles,
                                     MusaDwg1032OwnerWork *work,
                                     size_t work_count,
                                     uint64_t *roots,
                                     size_t *depths);
#ifdef __cplusplus
}
#endif
#endif
