/* SPDX-License-Identifier: MIT
 * Independently authored viewport candidate collector. No legacy CAD code.
 */
#include "musa_scene2d_cull.h"
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <stddef.h>

static int valid_box(MusaScene2dBox b) {
    return isfinite(b.min_x) && isfinite(b.min_y) &&
           isfinite(b.max_x) && isfinite(b.max_y) &&
           b.min_x <= b.max_x && b.min_y <= b.max_y;
}
static int overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}
/* All four affine corners bound a transformed rectangle, including
 * reflections, rotations, shears and negative scales. */
static int candidate(const MusaScene2dNode *node,
                     const MusaScene2dResolved *r, MusaScene2dBox local,
                     MusaScene2dBox view, int *hit) {
    if (!node->handle || node->handle != r->handle ||
        r->visible > 1 ||
        (node->kind != MUSA_SCENE2D_INSERT &&
         node->kind != MUSA_SCENE2D_ENTITY)) return 0;
    for (size_t j = 0; j < 6; ++j)
        if (!isfinite(r->world[j])) return 0;
    *hit = 0;
    if (node->kind == MUSA_SCENE2D_INSERT) return 1;
    if (!valid_box(local)) return 0;
    if (!r->visible) return 1;
    double min_x = INFINITY, min_y = INFINITY;
    double max_x = -INFINITY, max_y = -INFINITY;
    for (unsigned j = 0; j < 4; ++j) {
        const double x = (j & 1u) ? local.max_x : local.min_x;
        const double y = (j & 2u) ? local.max_y : local.min_y;
        const double wx = r->world[0]*x + r->world[2]*y + r->world[4];
        const double wy = r->world[1]*x + r->world[3]*y + r->world[5];
        if (!isfinite(wx) || !isfinite(wy)) return 0;
        if (wx < min_x) min_x = wx;
        if (wx > max_x) max_x = wx;
        if (wy < min_y) min_y = wy;
        if (wy > max_y) max_y = wy;
    }
    /* Widen one representable step without introducing infinite bounds
     * at +/-DBL_MAX, consistent with the cached spatial index. */
    min_x = nextafter(min_x, -DBL_MAX);
    max_x = nextafter(max_x, DBL_MAX);
    min_y = nextafter(min_y, -DBL_MAX);
    max_y = nextafter(max_y, DBL_MAX);
    *hit = min_x <= view.max_x && max_x >= view.min_x &&
           min_y <= view.max_y && max_y >= view.min_y;
    return 1;
}
int musa_scene2d_collect_visible(
    const MusaScene2dNode *nodes, const MusaScene2dResolved *resolved,
    const MusaScene2dBox *local_bounds, size_t count, MusaScene2dBox viewport,
    size_t *indices, size_t capacity, size_t *written) {
    if (!written || !valid_box(viewport) ||
        (count && (!nodes || !resolved || !local_bounds || !indices)) ||
        count > SIZE_MAX / sizeof(*nodes) ||
        count > SIZE_MAX / sizeof(*resolved) ||
        count > SIZE_MAX / sizeof(*local_bounds) ||
        capacity > SIZE_MAX / sizeof(*indices)) return 0;
    const size_t out_bytes = capacity * sizeof(*indices);
    const size_t nodes_bytes = count * sizeof(*nodes);
    const size_t resolved_bytes = count * sizeof(*resolved);
    const size_t boxes_bytes = count * sizeof(*local_bounds);
    if (overlap(indices, out_bytes, nodes, nodes_bytes) ||
        overlap(indices, out_bytes, resolved, resolved_bytes) ||
        overlap(indices, out_bytes, local_bounds, boxes_bytes) ||
        overlap(indices, out_bytes, written, sizeof(*written)) ||
        overlap(written, sizeof(*written), nodes, nodes_bytes) ||
        overlap(written, sizeof(*written), resolved, resolved_bytes) ||
        overlap(written, sizeof(*written), local_bounds, boxes_bytes))
        return 0;
    size_t hits = 0;
    for (size_t i = 0; i < count; ++i) {
        int hit = 0;
        if (!candidate(&nodes[i], &resolved[i], local_bounds[i],
                       viewport, &hit)) return 0;
        if (hit) {
            if (hits == capacity) return 0;
            ++hits;
        }
    }
    size_t out_count = 0;
    for (size_t i = 0; i < count; ++i) {
        int hit = 0;
        /* Input has already been validated; no failure path after writes. */
        (void)candidate(&nodes[i], &resolved[i], local_bounds[i],
                        viewport, &hit);
        if (hit) indices[out_count++] = i;
    }
    *written = hits;
    return 1;
}
