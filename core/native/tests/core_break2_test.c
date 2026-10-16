/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_break2.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(double x,double y,double eps){assert(isfinite(x)&&fabs(x-y)<=eps);}
int main(void){
    const double line[]={0,0,10,0};
    const double rev[]={10,0,0,0};
    const double tiny[]={0,0,0.0001,0};
    const double flat[]={0,0,0,0};
    const double offset[]={1e9,1e9,1e9+0.0001,1e9};
    MusaCoreBreak2 result;
    assert(musa_core_break2_point(line,5,3,&result));
    near(result.first[0],0,1e-12);near(result.first[2],5,1e-12);
    near(result.second[0],5,1e-12);near(result.second[2],10,1e-12);
    assert(musa_core_break2_point(rev,5,3,&result));
    near(result.first[2],5,1e-12);near(result.second[2],0,1e-12);
    assert(musa_core_break2_point(tiny,0.00005,0,&result));
    near(result.first[2],0.00005,1e-12);
    MusaCoreBreak2 saved=result;
    assert(!musa_core_break2_point(line,0,0,&result));
    assert(!musa_core_break2_point(line,10,0,&result));
    assert(!musa_core_break2_point(line,10.1,0,&result));
    assert(!musa_core_break2_point(flat,0,0,&result));
    assert(!musa_core_break2_point(line,NAN,0,&result));
    assert(!musa_core_break2_point(line,INFINITY,0,&result));
    assert(!musa_core_break2_point(offset,1e9+0.00005,1e9,&result));
    assert(!musa_core_break2_point(line,2e12,1,&result));
    assert(result.first[2]==saved.first[2]&&result.second[0]==saved.second[0]);
    assert(musa_core_break2_gap(line,3,2,7,-1,&result));
    near(result.first[2],3,1e-12);near(result.second[0],7,1e-12);
    assert(musa_core_break2_gap(line,7,-1,3,2,&result));
    near(result.first[2],3,1e-12);near(result.second[0],7,1e-12);
    assert(musa_core_break2_gap(rev,7,-1,3,2,&result));
    near(result.first[2],7,1e-12);near(result.second[0],3,1e-12);
    assert(musa_core_break2_gap(tiny,0.00003,0,0.00007,0,&result));
    near(result.first[2],0.00003,1e-12);
    saved=result;
    assert(!musa_core_break2_gap(line,5,0,5,0,&result));
    assert(!musa_core_break2_gap(line,-1,0,5,0,&result));
    assert(!musa_core_break2_gap(line,5,0,11,0,&result));
    assert(!musa_core_break2_gap(line,0.0001,0,5,0,&result));
    assert(!musa_core_break2_gap(line,5,0,5.0001,0,&result));
    assert(!musa_core_break2_gap(line,NAN,0,7,0,&result));
    assert(!musa_core_break2_gap(flat,1,0,2,0,&result));
    assert(!musa_core_break2_gap(offset,1e9+0.00003,1e9,
                                 1e9+0.00007,1e9,&result));
    assert(result.first[2]==saved.first[2]&&result.second[0]==saved.second[0]);
    puts("2D BREAK source segment geometry and BREAK2 gap OK");
    return 0;
}
