/* SPDX-License-Identifier: MIT
 * First-party conservative spatial index for already prepared 2D bounds.
 * No DWG parsing, legacy CAD source, or external dependency.
 */
#ifndef MUSA_SCENE2D_INDEX_H
#define MUSA_SCENE2D_INDEX_H
#include "musa_scene2d_cache.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaScene2dIndexNode {
    MusaScene2dBox box;
    uint8_t occupied;
} MusaScene2dIndexNode;
/* Builds a complete binary union-AABB tree in caller-provided storage.
 * Storage needs 2 * next_power_of_two(count) nodes (index 0 unused).
 * The input order is retained; hidden entries do not occupy leaves.
 * All validation occurs before writes. On failure, storage and *leaf_count
 * remain unchanged. For count==0, returns leaf_count=0 without storage.
 * Rebuild whenever prepared bounds/visibility change. */
int musa_scene2d_index_build(const MusaScene2dPreparedBounds *prepared,
                             size_t count, MusaScene2dIndexNode *storage,
                             size_t capacity, size_t *leaf_count);
/* Returns viewport candidates in original input order, including edge
 * touches. Prunes entire disjoint subtrees. In the worst case this remains
 * O(count); spatial locality in input order improves pruning. Two passes
 * ensure indices and *written are unchanged on capacity failure.
 * The tree must be an unmodified successful build result. */
int musa_scene2d_index_query(const MusaScene2dIndexNode *storage,
                             size_t count, size_t leaf_count,
                             MusaScene2dBox viewport, size_t *indices,
                             size_t capacity, size_t *written);
/* Memory-bounded, ordered viewport pagination for very large scenes.
 * Returns up to capacity hits after skipping the first skip hits; *total_hits
 * reports all matching entities even when capacity is zero. Output and both
 * counts remain unchanged on invalid arguments. No heap allocation.
 * Each page performs a pruned tree walk; sequential page retrieval may
 * revisit nodes. The tree must be an unmodified successful build result. */
int musa_scene2d_index_query_page(const MusaScene2dIndexNode *storage,
                                  size_t count, size_t leaf_count,
                                  MusaScene2dBox viewport, size_t skip,
                                  size_t *indices, size_t capacity,
                                  size_t *written, size_t *total_hits);
#ifdef __cplusplus
}
#endif
#endif
