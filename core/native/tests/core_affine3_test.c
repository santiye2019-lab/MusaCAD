/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_affine3.h"
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
static void near(double a,double b,double e){assert(isfinite(a)&&fabs(a-b)<=e);}
static void point(const MusaCoreAffine3 *a,double x,double y,double z,
                  double tx,double ty,double tz,double tol){
    double r[3]={0};
    assert(musa_core_affine3_map_point(a,x,y,z,r));
    near(r[0],tx,tol);near(r[1],ty,tol);near(r[2],tz,tol);
}
int main(void){
    MusaCoreAffine3 identity=musa_core_affine3_identity();
    point(&identity,2,-3,7,2,-3,7,0.);
    MusaCoreAffine3 local={{0,-2,0,10, 3,0,0,-20, 0,0,-4,30}};
    MusaCoreAffine3 parent={{1,0,0,1000000, 0,1,0,-2000000, 0,0,1,3000000}};
    MusaCoreAffine3 composed={0};
    assert(musa_core_affine3_compose(&parent,&local,&composed));
    point(&local,1,2,3,6,-17,18,0.);
    point(&composed,1,2,3,1000006,-2000017,3000018,0.);
    /* A vector must NOT pick up translation. */
    double v[3]={0};assert(musa_core_affine3_map_vector(&composed,1,2,3,v));
    near(v[0],-4,0);near(v[1],3,0);near(v[2],-12,0);
    /* Nested INSERT order and reverse WCS hit-testing. */
    MusaCoreAffine3 reversed={0};
    assert(musa_core_affine3_compose(&local,&parent,&reversed));
    double different[3];assert(musa_core_affine3_map_point(&reversed,1,2,3,different));
    assert(different[0]!=1000006);
    MusaCoreAffine3 inverse={0};
    assert(musa_core_affine3_inverse(&composed,&inverse));
    point(&inverse,1000006,-2000017,3000018,1,2,3,1e-8);
    MusaCoreAffine3 product={0};
    assert(musa_core_affine3_compose(&composed,&inverse,&product));
    point(&product,1,2,3,1,2,3,1e-8);
    /* Reflection changes orientation, and preserves inverse reachability. */
    double det=0;
    assert(musa_core_affine3_determinant(&local,&det));
    near(det,-24,1e-12);
    assert(musa_core_affine3_determinant(&identity,&det));
    near(det,1,0);
    /* Shear/nonuniform scales are invertible even with mixed units. */
    MusaCoreAffine3 shear={{1,2,0,5, 0,1e-8,0,7, 0,0,1e8,9}};
    MusaCoreAffine3 undo;
    assert(musa_core_affine3_inverse(&shear,&undo));
    double forward[3],back[3];
    assert(musa_core_affine3_map_point(&shear,.5,.75,.25,forward));
    assert(musa_core_affine3_map_point(&undo,forward[0],forward[1],forward[2],back));
    near(back[0],.5,1e-5);near(back[1],.75,1e-6);near(back[2],.25,1e-8);
    /* Never overwrite valid output when transform is unusable. */
    MusaCoreAffine3 singular={{1,0,0,0, 1,0,0,0, 0,0,1,0}};
    MusaCoreAffine3 ill={{1,1,0,0, 1,1+1e-15,0,0, 0,0,1,0}};
    MusaCoreAffine3 sentinel=musa_core_affine3_identity();
    assert(!musa_core_affine3_inverse(&singular,&sentinel));
    assert(!musa_core_affine3_inverse(&ill,&sentinel));
    near(sentinel.m[0],1,0);near(sentinel.m[5],1,0);
    MusaCoreAffine3 invalid=identity;invalid.m[0]=NAN;
    assert(!musa_core_affine3_compose(&invalid,&local,&sentinel));
    assert(!musa_core_affine3_inverse(&invalid,&sentinel));
    double unchanged[3]={123,456,789};
    assert(!musa_core_affine3_map_point(&invalid,1,2,3,unchanged));
    assert(!musa_core_affine3_map_point(&identity,NAN,2,3,unchanged));
    near(unchanged[0],123,0);near(unchanged[1],456,0);near(unchanged[2],789,0);
    MusaCoreAffine3 overflow={{DBL_MAX,0,0,0,0,1,0,0,0,0,1,0}};
    assert(!musa_core_affine3_map_point(&overflow,2,0,0,unchanged));
    assert(!musa_core_affine3_compose(&overflow,&overflow,&sentinel));
    near(sentinel.m[0],1,0);
    puts("MusaCAD shared 3D WCS affine, nested compose, inverse, mirror tests PASS");
    return 0;
}
