/* SPDX-License-Identifier: MIT
 * Independent, conservative 2D viewport candidate collection (2026-10-28).
 */
#ifndef MUSA_SCENE2D_CULL_H
#define MUSA_SCENE2D_CULL_H
#include "musa_scene2d.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaScene2dBox {
    double min_x, min_y, max_x, max_y;
} MusaScene2dBox;
/* The three input arrays have identical order. INSERTs are not drawable;
 * their local_bounds entries are ignored. Every ENTITY requires a finite,
 * ordered local AABB. Returns indices of visible entities whose affine-
 * transformed AABB intersects viewport (including touching edges).
 * Conservative candidates only: no exact geometry clipping or DWG parsing.
 * O(count), no allocation. On any failure leaves indices and *written
 * unchanged. Rejects aliasing between output and input memory.
 */
int musa_scene2d_collect_visible(
    const MusaScene2dNode *nodes, const MusaScene2dResolved *resolved,
    const MusaScene2dBox *local_bounds, size_t count, MusaScene2dBox viewport,
    size_t *indices, size_t capacity, size_t *written);
#ifdef __cplusplus
}
#endif
#endif
