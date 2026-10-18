/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_ucs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
static void near(double a,double b){
    assert(isfinite(a)&&fabs(a-b)<=1e-8);
}
static void same3(const double p[3],double x,double y,double z){
    near(p[0],x);near(p[1],y);near(p[2],z);
}
int main(void){
    const double world[9]={0,0,0, 1,0,0, 0,1,0};
    const double front[9]={0,0,0, 1,0,0, 0,0,1};
    const double right[9]={0,0,0, 0,1,0, 0,0,1};
    const double custom[9]={10,20,30, 0,2,0, 0,0,3};
    const double oblique[9]={-5,6,7, 1,1,1, -1,1,0};
    MusaCoreUcsFrame f={0};
    double out[3]={-77,-77,-77};
    assert(musa_core_ucs_create(world,&f));
    const double at[3]={2,3,4};
    assert(musa_core_ucs_to_world(&f,at,out));
    same3(out,2,3,4);
    assert(musa_core_ucs_to_local(&f,at,out));
    same3(out,2,3,4);
    assert(musa_core_ucs_move(&f,1,5,out));
    same3(out,0,5,0);
    assert(musa_core_ucs_create(front,&f));
    assert(musa_core_ucs_move(&f,1,4,out));
    same3(out,0,0,4);
    assert(musa_core_ucs_move(&f,2,4,out));
    same3(out,0,-4,0);
    assert(musa_core_ucs_create(right,&f));
    assert(musa_core_ucs_move(&f,0,5,out));
    same3(out,0,5,0);
    assert(musa_core_ucs_move(&f,1,5,out));
    same3(out,0,0,5);
    assert(musa_core_ucs_move(&f,2,5,out));
    same3(out,5,0,0);
    assert(musa_core_ucs_create(custom,&f));
    const double one_two_three[3]={1,2,3};
    assert(musa_core_ucs_to_world(&f,one_two_three,out));
    same3(out,13,21,32);
    const double mapped[3]={13,21,32};
    assert(musa_core_ucs_to_local(&f,mapped,out));
    same3(out,1,2,3);
    assert(musa_core_ucs_move(&f,2,4,out));
    same3(out,4,0,0);
    assert(musa_core_ucs_create(oblique,&f));
    const double xyz[3]={2,3,4};
    assert(musa_core_ucs_to_world(&f,xyz,out));
    double worldpoint[3]={out[0],out[1],out[2]};
    assert(musa_core_ucs_to_local(&f,worldpoint,out));
    same3(out,2,3,4);
    near(f.axis[0][0]*f.axis[1][0]+f.axis[0][1]*f.axis[1][1]+
         f.axis[0][2]*f.axis[1][2],0);
    MusaCoreUcsFrame saved=f;
    double badzero[9]={0,0,0,0,0,0,0,1,0};
    double parallel[9]={0,0,0,1,0,0,2,0,0};
    double large[9]={1000000001,0,0,1,0,0,0,1,0};
    double nan[9]={0,0,0,1,0,0,NAN,1,0};
    assert(!musa_core_ucs_create(badzero,&f));
    assert(!musa_core_ucs_create(parallel,&f));
    assert(!musa_core_ucs_create(large,&f));
    assert(!musa_core_ucs_create(nan,&f));
    near(saved.origin[0],f.origin[0]);
    assert(!musa_core_ucs_create(NULL,&f));
    double unchanged[3]={9,8,7};
    assert(!musa_core_ucs_to_world(&f,NULL,unchanged));
    same3(unchanged,9,8,7);
    assert(!musa_core_ucs_move(&f,3,10,unchanged));
    same3(unchanged,9,8,7);
    assert(!musa_core_ucs_move(&f,0,INFINITY,unchanged));
    const double too_big[3]={DBL_MAX,DBL_MAX,DBL_MAX};
    assert(!musa_core_ucs_to_world(&f,too_big,unchanged));
    same3(unchanged,9,8,7);
    puts("PASS: first-party UCS right-handed axes, WCS/local, moves, safe failures");
    return 0;
}
