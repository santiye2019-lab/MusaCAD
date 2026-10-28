/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_geometry.h"
#include "musa_scene2d_cache.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <math.h>
static int contains(MusaScene2dBox b,double x,double y) {
    return x>=b.min_x && x<=b.max_x && y>=b.min_y && y<=b.max_y;
}
int main(void) {
    const double pi=3.14159265358979323846;
    MusaScene2dBox b={111,222,333,444};
    assert(musa_scene2d_line_bounds(4,7,-2,-3,&b));
    assert(contains(b,4,7) && contains(b,-2,-3));
    assert(musa_scene2d_circle_bounds(2,3,5,&b));
    assert(contains(b,-3,3) && contains(b,7,3) && contains(b,2,-2));
    assert(musa_scene2d_arc_bounds(0,0,10,pi/4,3*pi/4,&b));
    assert(contains(b,0,10) && b.min_y>0 && b.max_x<10);
    assert(musa_scene2d_arc_bounds(0,0,10,7*pi/4,pi/4,&b));
    assert(contains(b,10,0) && b.min_x>0);
    assert(musa_scene2d_arc_bounds(0,0,10,0,2*pi,&b));
    assert(contains(b,0,-10) && contains(b,-10,0));
    assert(musa_scene2d_arc_bounds(0,0,10,pi/2,pi/2,&b));
    assert(contains(b,10*cos(pi/2),10*sin(pi/2)) && b.min_y>9);
    for(int i=0;i<5000;++i) {
        double start=-pi+(i%79)*0.11;
        double end=start+(i%71)*0.1;
        assert(musa_scene2d_arc_bounds(100,-200,2.5,start,end,&b));
        for(int j=0;j<=32;++j) {
            double a=start+(end-start)*j/32.0;
            assert(contains(b,100+2.5*cos(a),-200+2.5*sin(a)));
        }
    }
    MusaScene2dBox keep=b;
    assert(!musa_scene2d_circle_bounds(0,0,-1,&b));
    assert(!musa_scene2d_line_bounds(NAN,0,0,0,&b));
    assert(!musa_scene2d_arc_bounds(0,0,1,0,INFINITY,&b));
    assert(!musa_scene2d_circle_bounds(DBL_MAX,0,DBL_MAX,&b));
    assert(b.min_x==keep.min_x && b.max_y==keep.max_y);
    assert(!musa_scene2d_line_bounds(0,0,1,1,0));
    /* Analytic primitive bounds feed the independent block/viewport pipeline. */
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
    assert(musa_scene2d_circle_bounds(0,0,2,&local[1]));
    MusaScene2dResolved resolved[2];
    MusaScene2dPreparedBounds cache[2];
    size_t written = 0, indices[2] = {99,99};
    assert(musa_scene2d_resolve(nodes,2,layers,2,resolved,2,&written));
    assert(musa_scene2d_prepare_bounds(nodes,resolved,local,2,
                                       cache,2,&written));
    assert(written == 2 && cache[0].visible == 0 &&
           cache[1].visible == 1);
    assert(musa_scene2d_collect_prepared(cache,2,
             (MusaScene2dBox){14,23,14,23},indices,2,&written));
    assert(written == 1 && indices[0] == 1);
    assert(musa_scene2d_collect_prepared(cache,2,
             (MusaScene2dBox){50,50,60,60},indices,2,&written));
    assert(written == 0);
    return 0;
}
