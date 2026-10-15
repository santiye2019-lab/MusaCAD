/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_curve_bounds.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
static void eq(double got,double want,double tolerance){
    assert(isfinite(got)&&fabs(got-want)<=tolerance);
}
static void sample_inside(double cx,double cy,double ux,double uy,double vx,double vy,
                          double start,double sweep,MusaCoreCurveBounds b){
    for(int k=0;k<=2000;k++){
        double t=start+sweep*(double)k/2000.0;
        double x=cx+ux*cos(t)+vx*sin(t),y=cy+uy*cos(t)+vy*sin(t);
        const double eps=1e-7;
        assert(x>=b.min_x-eps&&x<=b.max_x+eps);
        assert(y>=b.min_y-eps&&y<=b.max_y+eps);
    }
}
int main(void){
    const double pi=acos(-1.0),tau=2*pi;
    MusaCoreCurveBounds b={77,88,99,111};
    /* First-quadrant positive ARC is not a complete CIRCLE. */
    assert(musa_core_curve_bounds(0,0,10,0,0,10,0,pi/2,&b));
    eq(b.min_x,0,1e-10);eq(b.min_y,0,1e-10);
    eq(b.max_x,10,1e-10);eq(b.max_y,10,1e-10);
    sample_inside(0,0,10,0,0,10,0,pi/2,b);
    /* Reverse direction: lower-right quarter. */
    assert(musa_core_curve_bounds(0,0,10,0,0,10,0,-pi/2,&b));
    eq(b.min_x,0,1e-10);eq(b.min_y,-10,1e-10);
    eq(b.max_x,10,1e-10);eq(b.max_y,0,1e-10);
    sample_inside(0,0,10,0,0,10,0,-pi/2,b);
    /* WCS rotated elliptical basis, nonuniform radii and large translation. */
    assert(musa_core_curve_bounds(1e8,-1e8,5,12,-8,3,-pi/4,3*pi/2,&b));
    sample_inside(1e8,-1e8,5,12,-8,3,-pi/4,3*pi/2,b);
    assert(b.min_x>1e8-10&&b.max_x<1e8+10);
    /* Full ellipse must still contain both x and y axis extrema. */
    assert(musa_core_curve_bounds(1e8,-1e8,5,12,-8,3,0,tau,&b));
    eq(b.min_x,1e8-hypot(5,8),1e-7);
    eq(b.max_y,-1e8+hypot(12,3),1e-7);
    sample_inside(1e8,-1e8,5,12,-8,3,0,tau,b);
    /* Wrap-around angles and clockwise long-sweep. */
    assert(musa_core_curve_bounds(3,-5,8,1,-1,5,5*pi/3,2*pi/3,&b));
    sample_inside(3,-5,8,1,-1,5,5*pi/3,2*pi/3,b);
    assert(musa_core_curve_bounds(3,-5,8,1,-1,5,pi/4,-3*pi/2,&b));
    sample_inside(3,-5,8,1,-1,5,pi/4,-3*pi/2,b);
    /* A failure must never reset caller bounds to the world origin. */
    MusaCoreCurveBounds preserve={4,5,6,7};b=preserve;
    assert(!musa_core_curve_bounds(0,0,1,0,0,1,0,0,&b));
    assert(!musa_core_curve_bounds(0,0,1,0,0,1,NAN,1,&b));
    assert(!musa_core_curve_bounds(0,0,1,0,0,1,0,1,NULL));
    assert(!musa_core_curve_bounds(0,0,1,0,0,1,0,tau+0.1,&b));
    assert(!musa_core_curve_bounds(DBL_MAX,0,DBL_MAX,0,0,1,0,tau,&b));
    eq(b.min_x,4,0);eq(b.max_y,7,0);
    puts("MusaCAD common exact arc/ellipse WCS bounds PASS");
    return 0;
}
