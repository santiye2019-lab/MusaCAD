/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_affine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near(double actual,double expected,double tolerance){
    assert(isfinite(actual));
    assert(fabs(actual-expected)<=tolerance);
}
static void original_and_reverse(MusaCoreAffine2 matrix,double x,double y){
    double xx=0,yy=0,rx=0,ry=0;
    musa_core_affine_map(matrix,x,y,&xx,&yy);
    MusaCoreAffine2 inverse;
    assert(musa_core_affine_inverse(matrix,&inverse));
    musa_core_affine_map(inverse,xx,yy,&rx,&ry);
    near(rx,x,1e-7);near(ry,y,1e-7);
    MusaCoreAffine2 round=musa_core_affine_multiply(matrix,inverse);
    double a,b;
    musa_core_affine_map(round,3,-8,&a,&b);
    near(a,3,1e-7);near(b,-8,1e-7);
}
int main(void){
    MusaCoreAffine2 I=musa_core_affine_identity();
    double x,y;
    musa_core_affine_map(I,12,-3,&x,&y);
    near(x,12,0);near(y,-3,0);

    /* Parent INSERT: scale X2,Y3 at (100,-50). Child: +90 rotation
     * and local (4,5) displacement. Preserve DXF nesting order. */
    MusaCoreAffine2 parent={2,0,0,3,100,-50};
    MusaCoreAffine2 child={0,1,-1,0,4,5};
    MusaCoreAffine2 nested=musa_core_affine_multiply(parent,child);
    musa_core_affine_map(nested,1,2,&x,&y);
    near(x,104,0);near(y,-32,0);
    double cx,cy,wx,wy;
    musa_core_affine_map(child,1,2,&cx,&cy);
    musa_core_affine_map(parent,cx,cy,&wx,&wy);
    near(x,wx,0);near(y,wy,0);
    musa_core_affine_map(musa_core_affine_multiply(child,parent),1,2,&wx,&wy);
    assert(fabs(x-wx)>1 || fabs(y-wy)>1);
    original_and_reverse(nested,1,2);
    original_and_reverse(parent,-29,17);
    original_and_reverse(child,8,4);

    /* MINSERT arrays and mirror reflections use the same common engine. */
    MusaCoreAffine2 mirror={-1,0,0,2,1e8,-1e8};
    original_and_reverse(mirror,10,20);
    musa_core_affine_map(mirror,10,20,&x,&y);
    near(x,1e8-10,1e-8);
    near(y,-1e8+40,1e-8);

    /* Reverse selection fails closed without mutating caller's output. */
    MusaCoreAffine2 unchanged={4,5,6,7,8,9};
    MusaCoreAffine2 singular={1,2,2,4,0,0};
    assert(!musa_core_affine_inverse(singular,&unchanged));
    near(unchanged.a,4,0);near(unchanged.ty,9,0);
    MusaCoreAffine2 nearly={1,0,0,1e-15,0,0};
    assert(!musa_core_affine_inverse(nearly,&unchanged));
    MusaCoreAffine2 broken={NAN,0,0,1,0,0};
    assert(!musa_core_affine_inverse(broken,&unchanged));
    MusaCoreAffine2 inf={1,0,0,1,INFINITY,0};
    assert(!musa_core_affine_inverse(inf,&unchanged));
    assert(!musa_core_affine_inverse(I,NULL));
    near(unchanged.a,4,0);near(unchanged.ty,9,0);
    puts("MusaCAD common affine: nested INSERT, MINSERT, scale, mirror, WCS inverse PASS");
    return 0;
}
