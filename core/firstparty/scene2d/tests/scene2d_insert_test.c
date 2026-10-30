/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_insert.h"
#include "musa_scene2d.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
static int near(double a, double b) { return fabs(a-b) < 1e-10; }
static void apply(const double a[6], double x, double y, double *ox, double *oy) {
    *ox = a[0]*x + a[2]*y + a[4];
    *oy = a[1]*x + a[3]*y + a[5];
}
static void basic(void) {
    const double halfpi = acos(-1.0)/2.0;
    double a[6], x, y;
    MusaScene2dInsertPlacement p = {10,20,2,3,1,1,0};
    assert(musa_scene2d_insert_affine(&p,a));
    assert(near(a[0],1) && near(a[3],1) && near(a[4],8) && near(a[5],17));
    apply(a,2,3,&x,&y); assert(near(x,10) && near(y,20));
    p.scale_x=2; p.scale_y=3; p.rotation_rad=halfpi;
    assert(musa_scene2d_insert_affine(&p,a));
    apply(a,2,3,&x,&y); assert(near(x,10) && near(y,20));
    apply(a,3,3,&x,&y); assert(near(x,10) && near(y,22));
    apply(a,2,4,&x,&y); assert(near(x,7) && near(y,20));
    p.scale_x=-2; p.scale_y=0; p.rotation_rad=0;
    assert(musa_scene2d_insert_affine(&p,a));
    apply(a,3,99,&x,&y); assert(near(x,8) && near(y,20));
}
static void hierarchy(void) {
    MusaScene2dNode nodes[3] = {
        {11,0,4,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,{0}},
        {12,11,0,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYBLOCK,{0}},
        {13,12,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,{1,0,0,1,2,3}}
    };
    MusaScene2dLayer layers[] = {{0,0xffeeeeeeu,1},{4,0xff123456u,1}};
    MusaScene2dResolved resolved[3]; size_t written=0;
    const MusaScene2dInsertPlacement outer = {10,20,2,3,2,1,0};
    const MusaScene2dInsertPlacement inner = {5,6,1,2,-1,2,0};
    assert(musa_scene2d_insert_affine(&outer,nodes[0].affine));
    assert(musa_scene2d_insert_affine(&inner,nodes[1].affine));
    assert(musa_scene2d_resolve(nodes,3,layers,2,resolved,3,&written));
    assert(written==3 && resolved[2].effective_layer==4);
    assert(resolved[2].rgba==0xff123456u && resolved[2].visible==1);
    double x,y; apply(resolved[2].world,1,1,&x,&y);
    /* Entity point (1,1) -> (3,4), inner -> (3,10), outer -> (12,27). */
    assert(near(x,12) && near(y,27));
}
/* Large, finite CAD coordinates must not be rejected because an
 * intermediate multiply overflows before cancellation with insertion. */
static void extreme_finite_translations(void) {
    MusaScene2dInsertPlacement p={
        1e308,1e308,2.0,2.0,1e308,1e308,0.0
    };
    double out[6]={0};
    assert(musa_scene2d_insert_affine(&p,out));
    assert(out[0]==1e308 && out[3]==1e308);
    assert(out[4]==-1e308 && out[5]==-1e308);
    double saved[6];
    memcpy(saved,out,sizeof(out));
    p.base_x=4.0; /* The final translation really is out of range. */
    assert(!musa_scene2d_insert_affine(&p,out));
    assert(memcmp(out,saved,sizeof(out))==0);
}
/* Rotated INSERT base offsets: the first evaluation order produces an
 * intermediate infinity, although the final world X is finite. */
static void extreme_rotated_base_cancellation(void) {
    MusaScene2dInsertPlacement p={
        0.0,1.5e308,1.0,2.0,1.4e308,1.4e308,acos(-1.0)/4.0
    };
    double out[6];
    assert(musa_scene2d_insert_affine(&p,out));
    assert(isfinite(out[4]) && isfinite(out[5]));
    const double expected_x=fma(-out[2],p.base_y,
                                -out[0]*p.base_x);
    const double expected_y=fma(-out[1],p.base_x,
                                fma(-out[3],p.base_y,p.insertion_y));
    assert(isfinite(expected_x) && isfinite(expected_y));
    assert(fabs((out[4]-expected_x)/expected_x)<1e-14);
    assert(fabs((out[5]-expected_y)/expected_y)<1e-14);
    double saved[6];
    memcpy(saved,out,sizeof(saved));
    p.base_y=4.0; /* Genuine final overflow remains a hard failure. */
    assert(!musa_scene2d_insert_affine(&p,out));
    assert(memcmp(out,saved,sizeof(saved))==0);
}
/* A reflected block must keep the sign of each axis through rotation. */
static void mirrored_rotated_insert(void) {
    const double halfpi=acos(-1.0)/2.0;
    MusaScene2dInsertPlacement p={10,20,2,3,-2,3,halfpi};
    double a[6],x,y;
    assert(musa_scene2d_insert_affine(&p,a));
    apply(a,2,3,&x,&y);
    assert(near(x,10) && near(y,20));
    apply(a,3,3,&x,&y);
    assert(near(x,10) && near(y,18));
    apply(a,2,4,&x,&y);
    assert(near(x,7) && near(y,20));
}
/* Reject a late invalid placement without publishing a partial page. */
static void page_overflow_rollback(void) {
    MusaScene2dInsertPlacement p[3]={
        {1,2,0,0,1,1,0},
        {3,4,0,0,1,1,0},
        {1e308,0,-1e308,0,2,1,0}
    };
    double out[3][6]={{-9},{-9},{-9}},saved[3][6];
    memcpy(saved,out,sizeof(out));
    size_t written=17,total=19;
    assert(!musa_scene2d_insert_affine_page(
        p,3,0,out,3,&written,&total));
    assert(written==17 && total==19);
    assert(memcmp(saved,out,sizeof(out))==0);
    assert(musa_scene2d_insert_affine_page(
        p,3,0,out,2,&written,&total));
    assert(written==2 && total==3);
}
static void rollback(void) {
    MusaScene2dInsertPlacement p={1,2,3,4,5,6,0};
    double out[6]={11,12,13,14,15,16},saved[6];
    memcpy(saved,out,sizeof out);
    assert(!musa_scene2d_insert_affine(NULL,out));
    assert(!musa_scene2d_insert_affine(&p,NULL));
    p.rotation_rad=NAN;
    assert(!musa_scene2d_insert_affine(&p,out));
    p.rotation_rad=0; p.scale_x=INFINITY;
    assert(!musa_scene2d_insert_affine(&p,out));
    p.scale_x=1e308; p.base_x=1e308;
    assert(!musa_scene2d_insert_affine(&p,out));
    p.scale_x=5; p.base_x=3;
    assert(!musa_scene2d_insert_affine(&p,(double *)(void *)&p));
    assert(memcmp(saved,out,sizeof out)==0);
}
static void many(void) {
    for (int i=0;i<20000;++i) {
        const double k=(double)(i%101-50)/7.0;
        MusaScene2dInsertPlacement p={k*3,k-4,k/2,k/3,
                                      (double)(i%9-4)/3.0,
                                      (double)(i%7-3)/2.0,k};
        double a[6],x,y;
        assert(musa_scene2d_insert_affine(&p,a));
        apply(a,p.base_x,p.base_y,&x,&y);
        assert(fabs(x-p.insertion_x)<1e-11);
        assert(fabs(y-p.insertion_y)<1e-11);
    }
}
static void bulk_placements(void) {
    enum { N=20000 };
    MusaScene2dInsertPlacement *placements=malloc(N*sizeof(*placements));
    double (*out)[6]=malloc(N*sizeof(*out));
    double (*saved)[6]=malloc(N*sizeof(*saved));
    assert(placements && out && saved);
    for (size_t i=0;i<N;++i) {
        placements[i]=(MusaScene2dInsertPlacement){
            (double)i, -(double)i, 1.0, 2.0, 1.0, -2.0, 0.0};
        for (size_t j=0;j<6u;++j) out[i][j]=-12345.0;
    }
    assert(musa_scene2d_insert_affine_batch(placements,N,out,N));
    for (size_t i=0;i<N;++i) {
        double x=0,y=0;
        apply(out[i],1.0,2.0,&x,&y);
        assert(near(x,(double)i) && near(y,-(double)i));
    }
    memcpy(saved,out,N*sizeof(*out));
    placements[N-1u].rotation_rad=NAN; /* Late failure: no partial output. */
    assert(!musa_scene2d_insert_affine_batch(placements,N,out,N));
    assert(memcmp(saved,out,N*sizeof(*out))==0);
    placements[N-1u].rotation_rad=0.0;
    assert(!musa_scene2d_insert_affine_batch(placements,N,out,N-1u));
    assert(!musa_scene2d_insert_affine_batch(placements,N,
               (double (*)[6])(void *)placements,N));
    assert(memcmp(saved,out,N*sizeof(*out))==0);
    assert(musa_scene2d_insert_affine_batch(NULL,0,NULL,0));
    assert(!musa_scene2d_insert_affine_batch(NULL,1,out,1));
    assert(!musa_scene2d_insert_affine_batch(placements,1,NULL,1));
    free(saved);free(out);free(placements);
}
static void paged_placements(void) {
    enum { N=20000, CAP=16 };
    MusaScene2dInsertPlacement *placements=malloc(N*sizeof(*placements));
    assert(placements);
    for (size_t i=0;i<N;++i)
        placements[i]=(MusaScene2dInsertPlacement){
            (double)i, (double)i*2.0, 0,0,1,1,0};
    double out[CAP][6],saved[CAP][6];
    for (size_t i=0;i<CAP;++i)
        for (size_t j=0;j<6u;++j) out[i][j]=-777.0;
    size_t written=123u,total=456u;
    assert(musa_scene2d_insert_affine_page(
        placements,N,N-5u,out,CAP,&written,&total));
    assert(written==5u && total==N);
    for (size_t i=0;i<5u;++i) {
        double x,y;
        apply(out[i],0,0,&x,&y);
        assert(near(x,(double)(N-5u+i)));
        assert(near(y,2.0*(double)(N-5u+i)));
    }
    for (size_t i=5u;i<CAP;++i)
        for (size_t j=0;j<6u;++j) assert(out[i][j]==-777.0);
    memcpy(saved,out,sizeof(out));
    placements[N-1u].scale_x=INFINITY;
    written=123u;total=456u;
    assert(!musa_scene2d_insert_affine_page(
        placements,N,N-7u,out,CAP,&written,&total));
    assert(written==123u && total==456u &&
           memcmp(saved,out,sizeof(out))==0);
    /* Only the requested page is parsed: unrelated later errors do not
     * make a visible early page unavailable. */
    assert(musa_scene2d_insert_affine_page(
        placements,N,0u,out,3u,&written,&total));
    assert(written==3u && total==N);
    memcpy(saved,out,sizeof(out));
    assert(!musa_scene2d_insert_affine_page(
        placements,N,0u,(double (*)[6])(void *)placements,
        3u,&written,&total));
    assert(!musa_scene2d_insert_affine_page(
        placements,N,0u,out,CAP,(size_t *)(void *)out,&total));
    assert(!musa_scene2d_insert_affine_page(
        placements,N,0u,out,CAP,&written,&written));
    assert(memcmp(saved,out,sizeof(out))==0);
    assert(musa_scene2d_insert_affine_page(
        placements,N,SIZE_MAX,NULL,0u,&written,&total));
    assert(written==0u && total==N);
    assert(musa_scene2d_insert_affine_page(
        placements,N,0u,NULL,0u,&written,&total));
    assert(written==0u && total==N);
    assert(musa_scene2d_insert_affine_page(
        NULL,0u,0u,NULL,0u,&written,&total));
    assert(written==0u && total==0u);
    free(placements);
}
int main(void) {basic();mirrored_rotated_insert();page_overflow_rollback();hierarchy();extreme_finite_translations();extreme_rotated_base_cancellation();rollback();many();bulk_placements();paged_placements();return 0;}
