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
    assert(!musa_dwg1032_affine2_apply(&t,p,0,out));
    assert(!musa_dwg1032_affine2_apply(&t,p,3,NULL));
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
/* Synthetic large-sheet coordinates: a finite result must not disappear
 * merely because the intermediate product sum exceeds double range. */
static void finite_translation_cancellation(void) {
    const double huge=1e308;
    const MusaDwg1032Affine2 outer={1.0,1.0,-huge,0.0,1.0,0.0};
    const MusaDwg1032Point2 point={huge,huge};
    MusaDwg1032Point2 out={-17.0,-19.0};
    assert(musa_dwg1032_affine2_apply(&outer,&point,1u,&out));
    assert(out.x==huge && out.y==huge);

    /* The inner fused sum can cancel a finite translation exactly. */
    const MusaDwg1032Affine2 balanced={1.0,-1.0,0.0,0.0,1.0,0.0};
    assert(musa_dwg1032_affine2_apply(&balanced,&point,1u,&out));
    assert(out.x==0.0 && out.y==huge);

    const MusaDwg1032Affine2 inner={1.0,0.0,huge,0.0,1.0,huge};
    MusaDwg1032Affine2 composed={0};
    assert(musa_dwg1032_affine2_compose(&outer,&inner,&composed));
    assert(composed.tx==huge && composed.ty==huge);
    const MusaDwg1032Point2 origin={0.0,0.0};
    assert(musa_dwg1032_affine2_apply(&composed,&origin,1u,&out));
    assert(out.x==huge && out.y==huge);

    /* A genuinely unrepresentable world coordinate must still fail
     * without publishing a partially updated output. */
    const MusaDwg1032Affine2 overflow={1.0,1.0,0.0,0.0,1.0,0.0};
    const MusaDwg1032Point2 saved=out;
    assert(!musa_dwg1032_affine2_apply(&overflow,&point,1u,&out));
    assert(memcmp(&out,&saved,sizeof out)==0);
}

/* Compare the one-pass scratch path against the two-pass transactional
 * baseline on a large synthetic sheet. Neither path parses DWG bytes. */
static void scratch_batch_and_atomicity(void) {
    enum { N=20000 };
    static MusaDwg1032Point2 input[N], baseline[N], output[N], scratch[N];
    const MusaDwg1032Point2 at={1000000.0,-2000000.0};
    MusaDwg1032Affine2 t;
    assert(musa_dwg1032_affine2_insert(&at,2.0,-3.0,0.0,&t));
    for (size_t i=0;i<N;++i) {
        input[i]=(MusaDwg1032Point2){(double)i,-(double)i};
        output[i]=(MusaDwg1032Point2){-7.0,-9.0};
    }
    assert(musa_dwg1032_affine2_apply(&t,input,N,baseline));
    assert(musa_dwg1032_affine2_apply_scratch(&t,input,N,output,scratch,N));
    assert(memcmp(baseline,output,sizeof(output))==0);
    /* A late invalid point must not publish any partial output. */
    input[N-1].x=NAN;
    assert(!musa_dwg1032_affine2_apply_scratch(&t,input,N,output,scratch,N));
    assert(memcmp(baseline,output,sizeof(output))==0);
    input[N-1].x=(double)(N-1);
    assert(!musa_dwg1032_affine2_apply_scratch(&t,input,N,output,scratch,N-1));
    assert(!musa_dwg1032_affine2_apply_scratch(&t,input,N,output,output,N));
    assert(!musa_dwg1032_affine2_apply_scratch(&t,input,N,output,input,N));
    assert(!musa_dwg1032_affine2_apply_scratch(&t,input,N,input,scratch,N));
    assert(memcmp(baseline,output,sizeof(output))==0);
    /* The scratch path must retain finite cancellation near DBL_MAX. */
    const MusaDwg1032Affine2 cancellation={1.0,1.0,-1e308,0.0,1.0,0.0};
    const MusaDwg1032Point2 extreme={1e308,1e308};
    MusaDwg1032Point2 result={-3.0,-4.0},temp={0.0,0.0};
    assert(musa_dwg1032_affine2_apply_scratch(
        &cancellation,&extreme,1u,&result,&temp,1u));
    assert(result.x==1e308 && result.y==1e308);
    const MusaDwg1032Affine2 overflow={1.0,1.0,0.0,0.0,1.0,0.0};
    assert(!musa_dwg1032_affine2_apply_scratch(
        &overflow,&extreme,1u,&result,&temp,1u));
    assert(result.x==1e308 && result.y==1e308);
}
static void reverse_insert_mapping(void) {
    const MusaDwg1032Point2 at={100.0,-50.0};
    MusaDwg1032Affine2 forward,reverse,original;
    assert(musa_dwg1032_affine2_insert(
        &at,-2.0,3.0,acos(-1.0)/3.0,&forward));
    assert(musa_dwg1032_affine2_inverse(&forward,&reverse));
    const MusaDwg1032Point2 local[3]={
        {0.0,0.0},{4.5,-2.0},{-100.0,75.0}};
    MusaDwg1032Point2 world[3],roundtrip[3];
    assert(musa_dwg1032_affine2_apply(&forward,local,3,world));
    assert(musa_dwg1032_affine2_apply(&reverse,world,3,roundtrip));
    for (size_t i=0;i<3u;++i) {
        assert(near(local[i].x,roundtrip[i].x));
        assert(near(local[i].y,roundtrip[i].y));
    }
    original=reverse;
    const MusaDwg1032Affine2 singular={1.0,2.0,0.0,2.0,4.0,0.0};
    assert(!musa_dwg1032_affine2_inverse(&singular,&reverse));
    assert(memcmp(&original,&reverse,sizeof reverse)==0);
    const MusaDwg1032Affine2 invalid={1.0,0.0,NAN,0.0,1.0,0.0};
    assert(!musa_dwg1032_affine2_inverse(&invalid,&reverse));
    assert(!musa_dwg1032_affine2_inverse(&forward,&forward));
    assert(memcmp(&original,&reverse,sizeof reverse)==0);
    /* The raw determinant underflows/overflows for these invertible
     * scales. Row-normalized inversion must preserve both directions. */
    const MusaDwg1032Affine2 tiny={1e-300,0.0,0.0,0.0,1e-300,0.0};
    assert(musa_dwg1032_affine2_inverse(&tiny,&reverse));
    assert(near(reverse.xx/1e300,1.0) && near(reverse.yy/1e300,1.0));
    const MusaDwg1032Affine2 huge={1e300,0.0,0.0,0.0,1e300,0.0};
    assert(musa_dwg1032_affine2_inverse(&huge,&reverse));
    assert(near(reverse.xx*1e300,1.0) && near(reverse.yy*1e300,1.0));
    const MusaDwg1032Affine2 mixed={1e-300,0.0,0.0,0.0,1e300,0.0};
    assert(musa_dwg1032_affine2_inverse(&mixed,&reverse));
    assert(near(reverse.xx/1e300,1.0) && near(reverse.yy*1e300,1.0));
    const MusaDwg1032Affine2 zero_row={0.0,0.0,0.0,0.0,1.0,0.0};
    original=reverse;
    assert(!musa_dwg1032_affine2_inverse(&zero_row,&reverse));
    assert(memcmp(&original,&reverse,sizeof reverse)==0);
}
/* Synthetic decoded block extents: world bounds must enclose all four
 * corners, not only the diagonal pair. No DWG record parsing is implied. */
static void block_bounds_and_viewport_culling(void) {
    const MusaDwg1032Bounds2 local={0.0,-1.0,2.0,3.0};
    const MusaDwg1032Affine2 shear={2.0,1.0,10.0,-1.0,3.0,-5.0};
    MusaDwg1032Bounds2 world={-7.0,-8.0,-9.0,-10.0};
    assert(musa_dwg1032_affine2_bounds(&shear,&local,&world));
    assert(near(world.min_x,9.0) && near(world.max_x,17.0));
    assert(near(world.min_y,-10.0) && near(world.max_y,4.0));
    unsigned char visible=9u;
    const MusaDwg1032Bounds2 touching={17.0,4.0,20.0,10.0};
    assert(musa_dwg1032_bounds2_intersects(&world,&touching,&visible));
    assert(visible==1u);
    const MusaDwg1032Bounds2 outside={17.01,4.0,20.0,10.0};
    assert(musa_dwg1032_bounds2_intersects(&world,&outside,&visible));
    assert(visible==0u);
    const MusaDwg1032Bounds2 inside={10.0,-2.0,11.0,0.0};
    assert(musa_dwg1032_bounds2_intersects(&world,&inside,&visible));
    assert(visible==1u);

    const MusaDwg1032Point2 insertion={100.0,200.0};
    MusaDwg1032Affine2 rotated;
    const MusaDwg1032Bounds2 rectangle={0.0,0.0,4.0,2.0};
    assert(musa_dwg1032_affine2_insert(
        &insertion,2.0,-3.0,acos(-1.0)/2.0,&rotated));
    assert(musa_dwg1032_affine2_bounds(&rotated,&rectangle,&world));
    assert(near(world.min_x,100.0) && near(world.max_x,106.0));
    assert(near(world.min_y,200.0) && near(world.max_y,208.0));

    const MusaDwg1032Bounds2 point={1.0,2.0,1.0,2.0};
    assert(musa_dwg1032_affine2_bounds(&shear,&point,&world));
    assert(near(world.min_x,14.0) && near(world.max_x,14.0));
    assert(near(world.min_y,0.0) && near(world.max_y,0.0));

    const MusaDwg1032Bounds2 saved=world;
    const MusaDwg1032Bounds2 inverted={3.0,0.0,2.0,1.0};
    const MusaDwg1032Bounds2 invalid={0.0,NAN,2.0,3.0};
    const MusaDwg1032Affine2 overflow={
        1e308,1e308,0.0,0.0,1.0,0.0
    };
    const MusaDwg1032Bounds2 huge={1e308,0.0,1e308,1e308};
    assert(!musa_dwg1032_affine2_bounds(&shear,&inverted,&world));
    assert(!musa_dwg1032_affine2_bounds(&shear,&invalid,&world));
    assert(!musa_dwg1032_affine2_bounds(&overflow,&huge,&world));
    assert(!musa_dwg1032_affine2_bounds(&shear,&world,&world));
    assert(!musa_dwg1032_affine2_bounds(NULL,&local,&world));
    assert(memcmp(&saved,&world,sizeof world)==0);
    visible=9u;
    assert(!musa_dwg1032_bounds2_intersects(&inverted,&inside,&visible));
    assert(!musa_dwg1032_bounds2_intersects(&inside,&invalid,&visible));
    assert(!musa_dwg1032_bounds2_intersects(
        &inside,&inside,(unsigned char *)(void *)&inside));
    assert(visible==9u);

    /* 20,000 independently placed block occurrences; bounded memory. */
    const MusaDwg1032Bounds2 unit={0.0,0.0,1.0,1.0};
    const MusaDwg1032Bounds2 viewport={5000.0,-1.0,6000.0,2.0};
    size_t hits=0u;
    for (size_t i=0u;i<20000u;++i) {
        const MusaDwg1032Affine2 translation={
            1.0,0.0,(double)i,0.0,1.0,0.0
        };
        assert(musa_dwg1032_affine2_bounds(&translation,&unit,&world));
        assert(musa_dwg1032_bounds2_intersects(
            &world,&viewport,&visible));
        hits+=(size_t)visible;
    }
    assert(hits==1002u);
}
int main(void) {
    instance_and_nested();
    reject_bad_and_atomic();
    large_sheet();
    finite_translation_cancellation();
    scratch_batch_and_atomicity();
    reverse_insert_mapping();
    block_bounds_and_viewport_culling();
    return 0;
}
