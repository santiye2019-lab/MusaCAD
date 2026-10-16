/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_corner2.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void eq(double a,double b){assert(fabs(a-b)<=1e-10);}
int main(void){
    const double a[]={0,0,10,0},b[]={0,0,0,10};
    const double revA[]={10,0,0,0},revB[]={0,10,0,0};
    const double parallel[]={0,1,10,1};
    const double external[]={2,0,10,0};
    MusaCoreCorner2 out;
    assert(musa_core_corner2_trim(a,b,5,0,0,5,2,0,&out));
    eq(out.ax,10);eq(out.ay,0);eq(out.atx,2);eq(out.aty,0);
    eq(out.bx,0);eq(out.by,10);eq(out.btx,0);eq(out.bty,2);
    eq(out.midx,1);eq(out.midy,1);
    assert(musa_core_corner2_trim(revA,revB,5,0,0,5,2,0,&out));
    eq(out.atx,2);eq(out.bty,2);
    assert(musa_core_corner2_trim(a,b,5,0,0,5,2,1,&out));
    eq(out.atx,2);eq(out.bty,2);
    eq(out.midx,2-2/sqrt(2));eq(out.midy,2-2/sqrt(2));
    MusaCoreCorner2 previous=out;
    assert(!musa_core_corner2_trim(a,b,5,0,0,5,12,1,&out));
    assert(!musa_core_corner2_trim(a,b,5,0,0,5,10,0,&out));
    assert(!musa_core_corner2_trim(external,b,5,0,0,5,2,0,&out));
    assert(!musa_core_corner2_trim(a,b,0,0,0,0,2,0,&out));
    assert(!musa_core_corner2_trim(a,parallel,5,0,5,1,2,1,&out));
    assert(!musa_core_corner2_trim(a,b,5,0,0,5,NAN,1,&out));
    eq(previous.ax,out.ax);eq(previous.ay,out.ay);
    eq(previous.atx,out.atx);eq(previous.aty,out.aty);
    eq(previous.bx,out.bx);eq(previous.by,out.by);
    eq(previous.btx,out.btx);eq(previous.bty,out.bty);
    eq(previous.midx,out.midx);eq(previous.midy,out.midy);
    puts("2D corner finite-segment gates OK");
    return 0;
}
