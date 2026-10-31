/* SPDX-License-Identifier: MIT
 * Synthetic INSERT transformations; not a real AC1032 DWG fixture.
 */
#include "musa_dwg1032_affine2d.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static int near(double a,double b) { return fabs(a-b)<1e-9; }
static void instance_and_nested(void) {
    const MusaDwg1032Point2 at={10,20}, nested_at={-2,3};
    MusaDwg1032Affine2 outer,inner,combined;
    const double quarter=acos(-1.0)/2.0;
    assert(musa_dwg1032_affine2_insert(&at,2,-3,quarter,&outer));
    assert(musa_dwg1032_affine2_insert(&nested_at,1,2,0,&inner));
    assert(musa_dwg1032_affine2_compose(&outer,&inner,&combined));
    const MusaDwg1032Point2 local[2]={{0,0},{1,2}};
    MusaDwg1032Point2 direct[2],first[2],second[2];
    assert(musa_dwg1032_affine2_apply(&combined,local,2,direct));
    assert(musa_dwg1032_affine2_apply(&inner,local,2,first));
    assert(musa_dwg1032_affine2_apply(&outer,first,2,second));
    for (size_t i=0;i<2;++i) {
        assert(near(direct[i].x,second[i].x));
        assert(near(direct[i].y,second[i].y));
    }
    assert(near(direct[0].x,19) && near(direct[0].y,16));
    const MusaDwg1032Point2 other_at={100,200};
    MusaDwg1032Affine2 other;
    assert(musa_dwg1032_affine2_insert(&other_at,2,-3,quarter,&other));
    assert(musa_dwg1032_affine2_apply(&other,local,2,second));
    assert(!near(second[0].x,direct[0].x));
}
static void reject_bad_and_atomic(void) {
    const MusaDwg1032Point2 at={1,2};
    MusaDwg1032Affine2 t,saved;
    assert(musa_dwg1032_affine2_insert(&at,1,1,0,&t));
    saved=t;
    assert(!musa_dwg1032_affine2_insert(&at,0,1,0,&t));
    assert(!musa_dwg1032_affine2_insert(&at,1,INFINITY,0,&t));
    assert(!musa_dwg1032_affine2_insert(&at,1,1,NAN,&t));
    assert(!musa_dwg1032_affine2_insert(&at,NAN,1,1,&t));
    assert(memcmp(&saved,&t,sizeof(t))==0);
    assert(!musa_dwg1032_affine2_compose(&t,&t,&t));
    MusaDwg1032Affine2 huge={1e308,0,0,0,1e308,0};
    assert(!musa_dwg1032_affine2_compose(&huge,&huge,&t));
    assert(memcmp(&saved,&t,sizeof(t))==0);
    MusaDwg1032Point2 p[3]={{1,2},{3,4},{5,6}},out[3];
    memset(out,0xa5,sizeof(out));
    MusaDwg1032Point2 before[3];
    memcpy(before,out,sizeof(out));
    p[2].x=NAN;
    assert(!musa_dwg1032_affine2_apply(&t,p,3,out));
    assert(memcmp(out,before,sizeof(out))==0);
    p[2].x=5;
    assert(!musa_dwg1032_affine2_apply(&t,p,3,p));
    assert(!musa_dwg1032_affine2_apply(&t,p,SIZE_MAX,out));
    assert(memcmp(out,before,sizeof(out))==0);
    assert(musa_dwg1032_affine2_apply(&t,p,3,out));
    assert(near(out[2].x,6) && near(out[2].y,8));
}
static void large_sheet(void) {
    enum { N=20000 };
    static MusaDwg1032Point2 p[N],out[N];
    const MusaDwg1032Point2 at={1000000,-2000000};
    MusaDwg1032Affine2 t;
    assert(musa_dwg1032_affine2_insert(&at,2,3,0,&t));
    for (size_t i=0;i<N;++i) p[i]=(MusaDwg1032Point2){(double)i,-(double)i};
    assert(musa_dwg1032_affine2_apply(&t,p,N,out));
    for (size_t i=0;i<N;++i) {
        assert(near(out[i].x,1000000+2*(double)i));
        assert(near(out[i].y,-2000000-3*(double)i));
    }
    p[N-1].y=INFINITY;
    assert(!musa_dwg1032_affine2_apply(&t,p,N,out));
    assert(near(out[N-1].x,1000000+2*(N-1)));
}
int main(void) {
    instance_and_nested();
    reject_bad_and_atomic();
    large_sheet();
    return 0;
}
