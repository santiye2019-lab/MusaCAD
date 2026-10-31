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

/* Predecoded Boolean visibility is inherited from every owner. The caller
 * must supply a previously validated, immutable object index and predecoded
 * owner handles and visibility bits (strictly 0 or 1). This does not parse
 * DWG layer or block metadata. Rejects dangling owners and cycles.
 * All buffers, including the full work capacity, must be disjoint.
 * On failure output remains unchanged; scratch contents are unspecified.
 * O(n log n), no allocation, no recursion. */
int musa_dwg1032_owner_graph_visibility(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *owner_handles, const uint8_t *local_visible,
    MusaDwg1032OwnerWork *work, size_t work_count,
    uint8_t *effective_visible);
/* A caller-verified layer table, sorted by strictly increasing nonzero
 * handles. on and thawed are strict Boolean values. This is not an AC1032
 * layer-table decoder. */
typedef struct MusaDwg1032LayerState {
    uint64_t handle;
    uint8_t on;
    uint8_t thawed;
} MusaDwg1032LayerState;

/* Combine predecoded per-object entity visibility, a verified layer table,
 * and inherited owner visibility. Every indexed object must have a nonzero
 * layer handle present in layers; callers must not fabricate layer 0 or
 * silently assume a missing layer is visible. Inputs and output/scratch
 * buffers must be pairwise disjoint, including unused work capacity.
 * local_work and work are caller-owned scratch; their contents may change
 * on failure. effective_visible is unchanged on any failure. No heap,
 * recursion, file parsing, viewport overrides, or DWG decompression.
 * O(n log n + n log layer_count) for n objects. */
int musa_dwg1032_owner_graph_layer_visibility(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *owner_handles,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    MusaDwg1032OwnerWork *work, size_t work_count,
    uint8_t *local_work, uint8_t *effective_visible);
#ifdef __cplusplus
}
#endif
#endif
