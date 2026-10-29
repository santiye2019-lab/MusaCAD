/* SPDX-License-Identifier: MIT
 * Independent decoded planar partial-ellipse regression coverage.
 */
#include "musa_scene2d_geometry.h"
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
    const double pi = 3.14159265358979323846;
    MusaScene2dBox b = {11,22,33,44};
    /* Axis-aligned quarter ellipse: extrema must be tight. */
    assert(musa_scene2d_ellipse_arc_bounds(0,0,4,0,0.5,0,pi/2,&b));
    assert(contains(b,4,0) && contains(b,0,2));
    assert(b.min_x > -1e-10 && b.min_y > -1e-10);
    assert(b.max_x < 4+1e-10 && b.max_y < 2+1e-10);
    /* A wrapped arc crossing the positive major axis is not full. */
    assert(musa_scene2d_ellipse_arc_bounds(0,0,4,0,0.5,
                                           7*pi/4,pi/4,&b));
    assert(contains(b,4,0) && b.min_x > 2.7 &&
           b.min_y < -1 && b.max_y > 1);
    /* Equal parameters represent a single point; 2*pi is full. */
    assert(musa_scene2d_ellipse_arc_bounds(0,0,4,0,0.5,
                                           pi/2,pi/2,&b));
    assert(contains(b,0,2) && b.min_y > 1.9);
    assert(musa_scene2d_ellipse_arc_bounds(0,0,4,0,0.5,0,2*pi,&b));
    assert(contains(b,-4,0) && contains(b,0,-2));
    /* Rotated and wrapped arcs: every sampled point must be contained.
     * The implementation is analytic; samples are test evidence only. */
    for (int i=0; i<1000; ++i) {
        const double start = -2*pi + 0.027*i;
        const double span = (i%127)*0.049;
        const double end = start + span;
        assert(musa_scene2d_ellipse_arc_bounds(100,-200,3,4,0.5,
                                               start,end,&b));
        for (int j=0; j<=64; ++j) {
            const double t = start + span*j/64.0;
            const double x = 100+3*cos(t)-2*sin(t);
            const double y = -200+4*cos(t)+1.5*sin(t);
            assert(contains(b,x,y));
        }
    }
    MusaScene2dBox keep=b;
    assert(!musa_scene2d_ellipse_arc_bounds(0,0,0,0,0.5,0,1,&b));
    assert(!musa_scene2d_ellipse_arc_bounds(0,0,1,0,0,0,1,&b));
    assert(!musa_scene2d_ellipse_arc_bounds(0,0,1,0,2,0,1,&b));
    assert(!musa_scene2d_ellipse_arc_bounds(0,0,1,0,0.5,NAN,1,&b));
    assert(!musa_scene2d_ellipse_arc_bounds(0,0,1,0,0.5,0,INFINITY,&b));
    assert(!musa_scene2d_ellipse_arc_bounds(DBL_MAX,0,DBL_MAX,0,
                                            0.5,0,1,&b));
    assert(!musa_scene2d_ellipse_arc_bounds(0,0,1,0,0.5,0,1,NULL));
    assert(b.min_x==keep.min_x && b.max_y==keep.max_y);
    /* Partial ellipse bounds pass through existing nested block transforms
     * and viewport candidate collection without a production DWG reader. */
    const MusaScene2dLayer layers[] = {
        {0,0xffffffffu,1}, {10,0xff00ff00u,1}
    };
    const MusaScene2dNode nodes[] = {
        {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,
         {1,0,0,1,10,20}},
        {200,100,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,
         {1,0,0,1,2,3}}
    };
    MusaScene2dBox local[2] = {{NAN,NAN,NAN,NAN},{0,0,0,0}};
    assert(musa_scene2d_ellipse_arc_bounds(0,0,4,0,0.5,
                                           0,pi/2,&local[1]));
    MusaScene2dResolved resolved[2];
    MusaScene2dPreparedBounds cache[2];
    size_t written=0, indices[2]={99,99};
    assert(musa_scene2d_resolve(nodes,2,layers,2,resolved,2,&written));
    assert(musa_scene2d_prepare_bounds(nodes,resolved,local,2,
                                       cache,2,&written));
    assert(musa_scene2d_collect_prepared(cache,2,
             (MusaScene2dBox){16,23,16,23},indices,2,&written));
    assert(written==1 && indices[0]==1);
    assert(musa_scene2d_collect_prepared(cache,2,
             (MusaScene2dBox){5,5,6,6},indices,2,&written));
    assert(written==0);
    return 0;
}
