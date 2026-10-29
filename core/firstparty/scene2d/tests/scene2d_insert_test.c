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
int main(void) {basic();hierarchy();rollback();many();return 0;}
