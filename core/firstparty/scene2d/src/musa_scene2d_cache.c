/* SPDX-License-Identifier: MIT
 * First-party 2D world-bound caching. No legacy CAD dependencies.
 */
#include "musa_scene2d_cache.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int finite_box(MusaScene2dBox b) {
    return isfinite(b.min_x) && isfinite(b.min_y) &&
           isfinite(b.max_x) && isfinite(b.max_y) &&
           b.min_x <= b.max_x && b.min_y <= b.max_y;
}
static int ordered_box(MusaScene2dBox b) {
    return !isnan(b.min_x) && !isnan(b.min_y) &&
           !isnan(b.max_x) && !isnan(b.max_y) &&
           b.min_x <= b.max_x && b.min_y <= b.max_y;
}
static int ranges_overlap(const void *a, size_t na, const void *b, size_t nb) {
    if (!na || !nb) return 0;
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < na : x - y < nb;
}
static int prepare_one(const MusaScene2dNode *n,
                       const MusaScene2dResolved *r, MusaScene2dBox local,
                       MusaScene2dPreparedBounds *dst) {
    if (!n->handle || n->handle != r->handle || r->visible > 1 ||
        (n->kind != MUSA_SCENE2D_ENTITY &&
         n->kind != MUSA_SCENE2D_INSERT)) return 0;
    for (size_t i = 0; i < 6; ++i)
        if (!isfinite(r->world[i])) return 0;
    dst->visible = 0;
    dst->box = (MusaScene2dBox){0,0,0,0};
    if (n->kind == MUSA_SCENE2D_INSERT) return 1;
    if (!finite_box(local)) return 0;
    if (!r->visible) return 1;
    double minx = INFINITY, miny = INFINITY;
    double maxx = -INFINITY, maxy = -INFINITY;
    for (unsigned k = 0; k < 4; ++k) {
        const double x = (k & 1u) ? local.max_x : local.min_x;
        const double y = (k & 2u) ? local.max_y : local.min_y;
        const double wx = r->world[0]*x + r->world[2]*y + r->world[4];
        const double wy = r->world[1]*x + r->world[3]*y + r->world[5];
        if (!isfinite(wx) || !isfinite(wy)) return 0;
        if (wx < minx) minx = wx;
        if (wx > maxx) maxx = wx;
        if (wy < miny) miny = wy;
        if (wy > maxy) maxy = wy;
    }
    /* One representable step guards against edge loss from rounding.
     * The widened bound may reach infinity near DBL_MAX. */
    dst->box = (MusaScene2dBox){
        nextafter(minx,-INFINITY), nextafter(miny,-INFINITY),
        nextafter(maxx,INFINITY), nextafter(maxy,INFINITY)
    };
    dst->visible = 1;
    return 1;
}
int musa_scene2d_prepare_bounds(
    const MusaScene2dNode *nodes, const MusaScene2dResolved *resolved,
    const MusaScene2dBox *local_bounds, size_t count,
    MusaScene2dPreparedBounds *out, size_t capacity, size_t *written) {
    if (!written || (count && (!nodes || !resolved || !local_bounds || !out)) ||
        capacity < count ||
        count > SIZE_MAX / sizeof(*nodes) ||
        count > SIZE_MAX / sizeof(*resolved) ||
        count > SIZE_MAX / sizeof(*local_bounds) ||
        count > SIZE_MAX / sizeof(*out)) return 0;
    const size_t bytes = count * sizeof(*out);
    if (ranges_overlap(out,bytes,nodes,count*sizeof(*nodes)) ||
        ranges_overlap(out,bytes,resolved,count*sizeof(*resolved)) ||
        ranges_overlap(out,bytes,local_bounds,count*sizeof(*local_bounds)) ||
        ranges_overlap(out,bytes,written,sizeof(*written)) ||
        ranges_overlap(written,sizeof(*written),nodes,count*sizeof(*nodes)) ||
        ranges_overlap(written,sizeof(*written),resolved,count*sizeof(*resolved)) ||
        ranges_overlap(written,sizeof(*written),local_bounds,count*sizeof(*local_bounds)))
        return 0;
    /* Validate every input before writing any output. */
    for (size_t i = 0; i < count; ++i) {
        MusaScene2dPreparedBounds value;
        if (!prepare_one(&nodes[i],&resolved[i],local_bounds[i],&value))
            return 0;
    }
    for (size_t i = 0; i < count; ++i)
        (void)prepare_one(&nodes[i],&resolved[i],local_bounds[i],&out[i]);
    *written = count;
    return 1;
}
int musa_scene2d_collect_prepared(
    const MusaScene2dPreparedBounds *prepared, size_t count,
    MusaScene2dBox viewport, size_t *indices, size_t capacity,
    size_t *written) {
    if (!written || !finite_box(viewport) ||
        (count && (!prepared || !indices)) ||
        count > SIZE_MAX / sizeof(*prepared) ||
        capacity > SIZE_MAX / sizeof(*indices)) return 0;
    if (ranges_overlap(indices,capacity*sizeof(*indices),
                       prepared,count*sizeof(*prepared)) ||
        ranges_overlap(indices,capacity*sizeof(*indices),
                       written,sizeof(*written)) ||
        ranges_overlap(written,sizeof(*written),
                       prepared,count*sizeof(*prepared))) return 0;
    size_t hits = 0;
    for (size_t i = 0; i < count; ++i) {
        const MusaScene2dPreparedBounds *p = &prepared[i];
        if (p->visible > 1 || (p->visible && !ordered_box(p->box)))
            return 0;
        if (p->visible && p->box.min_x <= viewport.max_x &&
            p->box.max_x >= viewport.min_x &&
            p->box.min_y <= viewport.max_y &&
            p->box.max_y >= viewport.min_y) {
            if (hits == capacity) return 0;
            ++hits;
        }
    }
    size_t n = 0;
    for (size_t i = 0; i < count; ++i) {
        const MusaScene2dPreparedBounds *p = &prepared[i];
        if (p->visible && p->box.min_x <= viewport.max_x &&
            p->box.max_x >= viewport.min_x &&
            p->box.min_y <= viewport.max_y &&
            p->box.max_y >= viewport.min_y) indices[n++] = i;
    }
    *written = hits;
    return 1;
}
