/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_array.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static int near(double a,double b) { return fabs(a-b)<1e-10; }
static void at(const double a[6],double x,double y,double *ox,double *oy) {
    *ox=a[0]*x+a[2]*y+a[4]; *oy=a[1]*x+a[3]*y+a[5];
}
int main(void) {
    const double halfpi=acos(-1.0)/2.0;
    MusaScene2dInsertArray a={{10,20,2,3,2,-3,halfpi},3,2,5,7};
    double m[6],x,y;
    assert(musa_scene2d_array_affine(&a,0,0,m));
    at(m,2,3,&x,&y); assert(near(x,10)&&near(y,20));
    assert(musa_scene2d_array_affine(&a,2,1,m));
    at(m,2,3,&x,&y);
    /* Local (10,7) offset -> scale (20,-21) -> rotate (21,20). */
    assert(near(x,31)&&near(y,40));
    double saved[6]; memcpy(saved,m,sizeof saved);
    assert(!musa_scene2d_array_affine(&a,3,0,m));
    assert(!musa_scene2d_array_affine(&a,0,2,m));
    assert(!musa_scene2d_array_affine(NULL,0,0,m));
    assert(!musa_scene2d_array_affine(&a,0,0,NULL));
    assert(!musa_scene2d_array_affine(&a,0,0,(double *)(void *)&a));
    a.column_spacing=NAN;
    assert(!musa_scene2d_array_affine(&a,0,0,m));
    a.column_spacing=5; a.placement.scale_x=INFINITY;
    assert(!musa_scene2d_array_affine(&a,0,0,m));
    a.placement.scale_x=2; a.rows=0;
    assert(!musa_scene2d_array_affine(&a,0,0,m));
    a.rows=2; a.column_spacing=1e308;
    assert(!musa_scene2d_array_affine(&a,2,0,m));
    assert(memcmp(m,saved,sizeof saved)==0);

    /* The unfused product 2 * 1e308 overflows, but the final placement
     * 2 * 1e308 - 1e308 is representable. Preserve the real coordinate. */
    MusaScene2dInsertArray huge={{-1e308,0,0,0,2,1,0},2,1,1e308,0};
    assert(musa_scene2d_array_affine(&huge,1,0,m));
    assert(isfinite(m[4]) && fabs(m[4]/1e308-1.0)<1e-14);
    assert(m[5]==0.0);
    double bulk[12];
    size_t written=99, total=99;
    assert(musa_scene2d_array_fill(&huge,bulk,2,&written));
    assert(written==2 && bulk[4]==-1e308 && bulk[10]==m[4]);
    double page[6]={0};
    assert(musa_scene2d_array_fill_page(&huge,1,page,1,
                                         &written,&total));
    assert(written==1 && total==2 && memcmp(page,m,sizeof(m))==0);
    return 0;
}
