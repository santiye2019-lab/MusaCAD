/* SPDX-License-Identifier: MIT
 * Independent regression coverage for already-decoded polyline geometry.
 */
#include "musa_scene2d_polyline.h"
#include "musa_scene2d_cache.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>

static int contains(MusaScene2dBox b, double x, double y) {
    return x >= b.min_x && x <= b.max_x &&
           y >= b.min_y && y <= b.max_y;
}

int main(void) {
    MusaScene2dBox b = {91,92,93,94};
    const MusaScene2dPolylineVertex line[] = {
        {0,0,0}, {10,0,0}, {10,5,0}
    };
    assert(musa_scene2d_polyline_bounds(line,3,0,&b));
    assert(contains(b,0,0) && contains(b,10,5));
    assert(b.min_x < 0 && b.max_y > 5);

    /* Positive and negative semicircles lie on opposite chord sides. */
    const MusaScene2dPolylineVertex lower[] = {{0,0,1},{10,0,0}};
    assert(musa_scene2d_polyline_bounds(lower,2,0,&b));
    assert(contains(b,5,-5) && contains(b,0,0) &&
           contains(b,10,0) && b.max_y < 1);
    const MusaScene2dPolylineVertex upper[] = {{0,0,-1},{10,0,0}};
    assert(musa_scene2d_polyline_bounds(upper,2,0,&b));
    assert(contains(b,5,5) && b.min_y > -1);

    /* A major arc extends beyond the chord in both axes. */
    const MusaScene2dPolylineVertex major[] = {{0,0,2},{10,0,0}};
    assert(musa_scene2d_polyline_bounds(major,2,0,&b));
    assert(contains(b,-1.25,-3.75) && contains(b,11.25,-3.75));
    assert(contains(b,5,-10));

    /* Small bulges must not be flattened to the chord. */
    const MusaScene2dPolylineVertex tiny[] = {{0,0,1e-6},{10,0,0}};
    assert(musa_scene2d_polyline_bounds(tiny,2,0,&b));
    assert(contains(b,5,-5e-6));

    /* Closing segment uses the last vertex's bulge. */
    const MusaScene2dPolylineVertex closed[] = {
        {0,0,0}, {10,0,0}, {10,10,1}
    };
    assert(musa_scene2d_polyline_bounds(closed,3,0,&b));
    assert(contains(b,0,0) && contains(b,10,10));
    const MusaScene2dBox open_bounds = b;
    assert(musa_scene2d_polyline_bounds(closed,3,1,&b));
    assert(contains(b,0,0) && contains(b,10,10));
    assert(b.min_x <= open_bounds.min_x && b.max_x >= open_bounds.max_x);

    /* Polyline bounds must reach the same nested-block viewport pipeline
     * used by LINE, CIRCLE, ARC and ELLIPSE primitives. */
    const MusaScene2dLayer layers[] = {
        {0,0xffffffffu,1}, {10,0xff00ff00u,1}
    };
    const MusaScene2dNode nodes[] = {
        {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,
         {1,0,0,1,100,200}},
        {200,100,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,
         {1,0,0,1,3,4}}
    };
    MusaScene2dBox local[] = {{NAN,NAN,NAN,NAN},{0,0,0,0}};
    assert(musa_scene2d_polyline_bounds(lower,2,0,&local[1]));
    MusaScene2dResolved resolved[2];
    MusaScene2dPreparedBounds prepared[2];
    size_t written = 0, indices[2] = {99,99};
    assert(musa_scene2d_resolve(nodes,2,layers,2,resolved,2,&written));
    assert(musa_scene2d_prepare_bounds(nodes,resolved,local,2,
                                       prepared,2,&written));
    assert(musa_scene2d_collect_prepared(prepared,2,
             (MusaScene2dBox){108,199,108,199},indices,2,&written));
    assert(written == 1 && indices[0] == 1);
    assert(musa_scene2d_collect_prepared(prepared,2,
             (MusaScene2dBox){108,205,108,205},indices,2,&written));
    assert(written == 0);

    /* Failure never mutates the previous output. */
    const MusaScene2dBox keep = b;
    const MusaScene2dPolylineVertex bad[] = {{0,0,1},{0,0,0}};
    assert(!musa_scene2d_polyline_bounds(bad,2,0,&b));
    assert(!musa_scene2d_polyline_bounds(line,1,0,&b));
    assert(!musa_scene2d_polyline_bounds(line,3,2,&b));
    assert(!musa_scene2d_polyline_bounds(NULL,3,0,&b));
    assert(!musa_scene2d_polyline_bounds(line,3,0,NULL));
    const MusaScene2dPolylineVertex nan[] = {{0,0,0},{NAN,1,0}};
    assert(!musa_scene2d_polyline_bounds(nan,2,0,&b));
    const MusaScene2dPolylineVertex overflow[] = {
        {-DBL_MAX,0,1},{DBL_MAX,0,0}
    };
    assert(!musa_scene2d_polyline_bounds(overflow,2,0,&b));
    assert(!musa_scene2d_polyline_bounds(line,3,0,
             (MusaScene2dBox *)(void *)&line[0]));
    assert(b.min_x == keep.min_x && b.min_y == keep.min_y &&
           b.max_x == keep.max_x && b.max_y == keep.max_y);
    return 0;
}
