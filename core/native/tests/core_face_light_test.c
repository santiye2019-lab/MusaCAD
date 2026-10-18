/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_face_light.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(double got,double wanted){
    assert(isfinite(got)&&fabs(got-wanted)<1e-12);
}
int main(void){
    const float xy[]={0,0,0,4,0,0,0,3,0};
    near(musa_core_face_lambert_f32(xy,3,0,1,2),.8);
    near(musa_core_face_lambert_f32(xy,3,2,1,0),.8);
    const float yz[]={0,0,0,0,3,0,0,0,4};
    near(musa_core_face_lambert_f32(yz,3,0,1,2),.35);
    const float huge[]={0,0,0,1e20f,0,0,0,1e20f,0};
    near(musa_core_face_lambert_f32(huge,3,0,1,2),.8);
    const float tiny[]={0,0,0,1e-12f,0,0,0,1e-12f,0};
    near(musa_core_face_lambert_f32(tiny,3,0,1,2),.8);
    const float world[]={1000000.f,1000000.f,0,1000004.f,1000000.f,0,1000000.f,1000003.f,0};
    near(musa_core_face_lambert_f32(world,3,0,1,2),.8);
    const float invalid[]={0,0,0,NAN,0,0,0,1,0};
    const float flat[]={0,0,0,1,1,1,2,2,2};
    assert(musa_core_face_lambert_f32(xy,3,0,0,2)<0.0);
    assert(musa_core_face_lambert_f32(flat,3,0,1,2)<0.0);
    assert(musa_core_face_lambert_f32(invalid,3,0,1,2)<0.0);
    assert(musa_core_face_lambert_f32(xy,3,0,1,3)<0.0);
    assert(musa_core_face_lambert_f32(NULL,3,0,1,2)<0.0);
    assert(musa_core_face_lambert_f32(xy,SIZE_MAX,0,1,2)<0.0);
    puts("PASS: shared first-party viewport face normals, huge/tiny & failure safety");
    return 0;
}
