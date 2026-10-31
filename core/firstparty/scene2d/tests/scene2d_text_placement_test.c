/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_text_placement.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static int near(double x,double y) { return fabs(x-y)<1e-10; }
static void point(const double a[6],double x,double y,double *ox,double *oy) {
    *ox=a[0]*x+a[2]*y+a[4];
    *oy=a[1]*x+a[3]*y+a[5];
}
int main(void) {
    MusaScene2dTextPlacement p={10,20,2,1,0,0,0,0};
    double a[6],x,y;
    assert(musa_scene2d_text_affine(&p,a));
    assert(near(a[0],2) && near(a[1],0) && near(a[2],0) &&
           near(a[3],2) && near(a[4],10) && near(a[5],20));
    p.anchor_x=3; p.anchor_y=1;
    assert(musa_scene2d_text_affine(&p,a));
    point(a,3,1,&x,&y); assert(near(x,10) && near(y,20));
    p.width_factor=-0.5;
    p.oblique_rad=atan(1.0);
    p.rotation_rad=acos(-1.0)/2;
    assert(musa_scene2d_text_affine(&p,a));
    point(a,3,1,&x,&y); assert(near(x,10) && near(y,20));
    point(a,4,1,&x,&y); assert(near(x,10) && near(y,19));
    point(a,3,2,&x,&y); assert(near(x,8) && near(y,22));
    /* A shaped unit-height glyph box (0,0)..(3,1), rotated 90 degrees. */
    p=(MusaScene2dTextPlacement){10,20,2,1,0,acos(-1.0)/2,0,0};
    assert(musa_scene2d_text_affine(&p,a));
    double minx=INFINITY,miny=INFINITY,maxx=-INFINITY,maxy=-INFINITY;
    for (int j=0;j<4;++j) {
        point(a,(j&1)?3:0,(j&2)?1:0,&x,&y);
        if (x<minx) minx=x;
        if (x>maxx) maxx=x;
        if (y<miny) miny=y;
        if (y>maxy) maxy=y;
    }
    assert(near(minx,8) && near(maxx,10) &&
           near(miny,20) && near(maxy,26));
    p.oblique_rad=atan(1.0); p.rotation_rad=0;
    assert(musa_scene2d_text_affine(&p,a));
    point(a,3,1,&x,&y);
    assert(near(x,18) && near(y,22));
    double saved[6]; memcpy(saved,a,sizeof(a));
    p.height=0; assert(!musa_scene2d_text_affine(&p,a));
    p.height=2; p.width_factor=0;
    assert(!musa_scene2d_text_affine(&p,a));
    p.width_factor=1; p.rotation_rad=NAN;
    assert(!musa_scene2d_text_affine(&p,a));
    p.rotation_rad=0; p.height=1e308; p.width_factor=1e308;
    assert(!musa_scene2d_text_affine(&p,a));
    /* Finite inputs can still underflow to a singular width scale. */
    p.height=1e-300; p.width_factor=1e-300;
    assert(!musa_scene2d_text_affine(&p,a));
    p.width_factor=-1e-300;
    assert(!musa_scene2d_text_affine(&p,a));
    p.height=2; p.width_factor=1;
    assert(!musa_scene2d_text_affine(&p,(double *)(void *)&p));
    assert(!musa_scene2d_text_affine(NULL,a));
    assert(!musa_scene2d_text_affine(&p,NULL));
    assert(memcmp(saved,a,sizeof(a))==0);
    /* Preserve valid tiny, but representable, nonzero glyph scales. */
    p=(MusaScene2dTextPlacement){0,0,1e-200,1e-100,0,0,0,0};
    assert(musa_scene2d_text_affine(&p,a));
    assert(a[0]>0.0 && a[3]>0.0);
    /* A very large anchor can cancel a large insertion coordinate.
     * A rounded product would lose the fractional residual (0.25). */
    p=(MusaScene2dTextPlacement){1e15+0.25,-1e15-0.25,
                                  1.0,0.1,0.0,0.0,
                                  1e16,-1e15-0.25};
    assert(musa_scene2d_text_affine(&p,a));
    assert(a[4]==fma(-a[0],p.anchor_x,p.insertion_x));
    assert(a[5]==0.0);
    /* A compiler may fuse the subtraction above under FP contraction.
     * Assert the preserved residual itself, not a non-fused expression. */
    assert(a[4]>0.125 && a[4]<0.25);
    /* With a rotated, anisotropically scaled TEXT, one anchor term
     * overflows before the opposite term cancels it. Both final
     * translations remain finite and must not be discarded. */
    p=(MusaScene2dTextPlacement){0,1.5e308,
        2.0*sqrt(2.0),0.5,0,acos(-1.0)/4.0,1e308,1e308};
    assert(musa_scene2d_text_affine(&p,a));
    assert(isfinite(a[4]) && fabs(a[4]/1e308-1.0)<1e-14);
    assert(isfinite(a[5]) && fabs(a[5]/1.5e308+1.0)<1e-14);
    memcpy(saved,a,sizeof(saved));
    p.insertion_y=-1.5e308; /* Genuine y overflow still rejects. */
    assert(!musa_scene2d_text_affine(&p,a));
    assert(memcmp(saved,a,sizeof(saved))==0);
    /* Mirrored text must retain a non-singular, signed x-axis. */
    p=(MusaScene2dTextPlacement){5,-7,2,-3,0,0,0,0};
    assert(musa_scene2d_text_affine(&p,a));
    assert(a[0]==-6 && a[3]==2 && a[4]==5 && a[5]==-7);
    memcpy(saved,a,sizeof(saved));
    /* Extreme oblique angles can overflow the shear despite finite input. */
    p.oblique_rad=1.5707963267948966;
    p.height=1e308;
    assert(!musa_scene2d_text_affine(&p,a));
    assert(memcmp(saved,a,sizeof(saved))==0);
    /* Large finite anchor offsets may produce an unrepresentable
     * translation; no partial affine matrix may escape on failure. */
    p=(MusaScene2dTextPlacement){1e308,0,2,1,0,0,-1e308,0};
    assert(!musa_scene2d_text_affine(&p,a));
    assert(memcmp(saved,a,sizeof(saved))==0);
    /* Nonfinite anchor data must be rejected before trigonometry. */
    p.anchor_x=INFINITY;
    assert(!musa_scene2d_text_affine(&p,a));
    assert(memcmp(saved,a,sizeof(saved))==0);
    for (int i=0;i<20000;++i) {
        const double k=(double)(i%101-50)/20.0;
        p=(MusaScene2dTextPlacement){k*3,k-4,2.0,0.1+(i%7)/4.0,
                                   k/5.0,k/3.0,k/2.0,k/4.0};
        assert(musa_scene2d_text_affine(&p,a));
        point(a,p.anchor_x,p.anchor_y,&x,&y);
        assert(near(x,p.insertion_x) && near(y,p.insertion_y));
    }
    return 0;
}
