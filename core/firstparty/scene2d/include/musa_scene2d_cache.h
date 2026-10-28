/* SPDX-License-Identifier: MIT
 * Independently authored cached world-space bounds for large 2D scenes.
 */
#ifndef MUSA_SCENE2D_CACHE_H
#define MUSA_SCENE2D_CACHE_H
#include "musa_scene2d_cull.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaScene2dPreparedBounds {
    MusaScene2dBox box; /* World-space conservative AABB. */
    uint8_t visible;    /* Only visible ENTITY nodes are drawable. */
} MusaScene2dPreparedBounds;
/* Cache affine-transformed AABBs once per resolved scene/viewport.
 * The cache must be rebuilt after changes to geometry, transforms,
 * visibility or viewport-layer freeze state. No DWG decoding is performed.
 * INSERTs and hidden entities have visible=0 and zero bounds.
 * Inputs and outputs may not alias. Failure preserves out and *written.
 */
int musa_scene2d_prepare_bounds(
    const MusaScene2dNode *nodes, const MusaScene2dResolved *resolved,
    const MusaScene2dBox *local_bounds, size_t count,
    MusaScene2dPreparedBounds *out, size_t capacity, size_t *written);
/* Repeated viewport queries use the cached world bounds, without affine
 * transformations. Results preserve input order; viewport edges are inclusive.
 * O(count), no allocation. On failure, indices and *written are unchanged.
 */
int musa_scene2d_collect_prepared(
    const MusaScene2dPreparedBounds *prepared, size_t count,
    MusaScene2dBox viewport, size_t *indices, size_t capacity,
    size_t *written);
#ifdef __cplusplus
}
#endif
#endif
