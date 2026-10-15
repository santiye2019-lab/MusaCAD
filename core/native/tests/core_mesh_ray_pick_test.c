/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
static void near(double x,double y,double e){assert(isfinite(x)&&fabs(x-y)<=e);}
int main(void) {
    const float pts[]={0,0,0, 2,0,0, 0,2,0,
                       0,0,-2, 2,0,-2, 0,2,-2};
    const int faces[]={0,1,2, 3,4,5};
    const int flipped[]={0,2,1};
    const double o[]={.5,.5,4.},d[]={0,0,-2};
    MusaCoreMeshRayHit hit={0};
    int rc=musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,d,10.,&hit);
    assert(rc==1&&hit.triangle_index==0&&hit.front_facing==1);
    near(hit.distance,4.,1e-12);
    near(hit.barycentric_u,.25,1e-12);
    near(hit.barycentric_v,.25,1e-12);
    near(hit.wcs_x,.5,1e-12);near(hit.wcs_y,.5,1e-12);
    near(hit.wcs_z,0,1e-12);

    assert(musa_core_mesh_ray_pick_f32(pts,6,flipped,1,o,d,10.,&hit)==1);
    assert(hit.triangle_index==0&&!hit.front_facing);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,d,3.9,&hit)==0);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,d,4.,&hit)==1);
    assert(hit.triangle_index==0);
    /* Skip coplanar/perpendicular direction: no made-up intersection. */
    const double sideways[]={1,0,0};
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,sideways,10.,&hit)==0);
    const double off[]={2.,2.,4.};
    hit.distance=123.;hit.triangle_index=999;
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,off,d,10.,&hit)==0);
    assert(hit.distance==123.&&hit.triangle_index==999);

    /* WCS northings/eastings far from the drawing origin. */
    const float world[]={1000000,1000000,1000000,
                         1000002,1000000,1000000,
                         1000000,1000002,1000000};
    const double wo[]={1000000.5,1000000.5,1000010};
    assert(musa_core_mesh_ray_pick_f32(world,3,faces,1,wo,d,10.,&hit)==1);
    near(hit.distance,10.,1e-12);
    near(hit.wcs_x,1000000.5,1e-9);
    near(hit.wcs_y,1000000.5,1e-9);
    near(hit.wcs_z,1000000,1e-9);
    /* A degenerately encoded triangle cannot be picked as a face. */
    const int deg[]={0,0,2};
    assert(musa_core_mesh_ray_pick_f32(pts,6,deg,1,o,d,10.,&hit)==0);
    /* Fail closed on invalid input, without publishing partial hits. */
    const int broken[]={0,1,2,0,1,99};
    const float nanmesh[]={0,0,0,2,NAN,0,0,2,0};
    const double badray[]={0,0,0},bado[]={0,0,NAN};
    hit.distance=4242.;hit.triangle_index=711;
    assert(musa_core_mesh_ray_pick_f32(pts,6,broken,2,o,d,10.,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(nanmesh,3,faces,1,o,d,10.,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,badray,10.,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,bado,d,10.,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,d,NAN,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,250001,o,d,10.,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(NULL,3,faces,1,o,d,10.,&hit)==-1);
    assert(musa_core_mesh_ray_pick_f32(pts,6,faces,2,o,d,10.,NULL)==-1);
    assert(hit.distance==4242.&&hit.triangle_index==711);
    assert(musa_core_mesh_ray_pick_f32(NULL,0,NULL,0,o,d,10.,&hit)==0);
    puts("MusaCAD C ABI 3D mesh ray selection: closest face/WCS/guards PASS");
    return 0;
}
