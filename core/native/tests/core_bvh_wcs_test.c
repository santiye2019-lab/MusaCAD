/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bvh_wcs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(double x,double y,double e){
    assert(isfinite(x)&&fabs(x-y)<=e);
}
int main(void){
    const float verts[]={0,0,0,2,0,0,0,2,0,
                         0,0,-1,2,0,-1,0,2,-1};
    const int faces[]={0,1,2,3,4,5};
    MusaCoreMeshBVH *b=NULL;
    assert(musa_core_mesh_bvh_build_f32(verts,6,faces,2,&b));
    const MusaCoreAffine3 id=musa_core_affine3_identity();
    const double o[]={.5,.5,5},d[]={0,0,-7};
    MusaCoreMeshRayHit local={0},world={0};
    MusaCoreMeshBVHStats stats={0};
    assert(musa_core_mesh_bvh_pick(b,o,d,10.,&local)==1);
    assert(musa_core_mesh_bvh_pick_wcs(b,&id,o,d,10.,&world,&stats)==1);
    assert(world.triangle_index==local.triangle_index);
    near(world.distance,local.distance,1e-12);
    near(world.wcs_x,local.wcs_x,1e-12);
    near(world.wcs_y,local.wcs_y,1e-12);
    near(world.wcs_z,local.wcs_z,1e-12);
    assert(world.front_facing==local.front_facing);
    assert(stats.tested_faces>0&&stats.visited_nodes>0);

    /* Mirrored, nonuniformly scaled, translated BLOCK instance. */
    const MusaCoreAffine3 reflected={{-2,0,0,10,
                                       0,3,0,20,
                                       0,0,4,30}};
    const double wo[]={9,21,38};
    const double wd[]={0,0,-5};
    assert(musa_core_mesh_bvh_pick_wcs(b,&reflected,wo,wd,10,
                                       &world,&stats)==1);
    assert(world.triangle_index==0);
    near(world.distance,8,1e-10);
    near(world.wcs_x,9,1e-10);
    near(world.wcs_y,21,1e-10);
    near(world.wcs_z,30,1e-10);
    near(world.barycentric_u,.25,1e-10);
    near(world.barycentric_v,1./6.,1e-10);
    assert(!world.front_facing);
    assert(musa_core_mesh_bvh_pick_wcs(b,&reflected,wo,wd,7.9,
                                       &world,&stats)==0);

    /* Nested INSERT transforms and large WCS coordinates, same BVH. */
    const MusaCoreAffine3 translated={{1,0,0,1000000,
                                        0,1,0,-2000000,
                                        0,0,1,3000000}};
    MusaCoreAffine3 nested;
    assert(musa_core_affine3_compose(&translated,&reflected,&nested));
    const double far_origin[]={1000009,-1999979,3000038};
    assert(musa_core_mesh_bvh_pick_wcs(b,&nested,far_origin,wd,10,
                                       &world,&stats)==1);
    near(world.wcs_x,1000009,1e-8);
    near(world.wcs_y,-1999979,1e-8);
    near(world.wcs_z,3000030,1e-8);
    near(world.distance,8,1e-8);

    /* Sheared building-block instance with nonuniform XYZ scaling. */
    const MusaCoreAffine3 shear={{2,.5,0,100,
                                  0,3,0,200,
                                  0,0,4,300}};
    const double sh_origin[]={101.25,201.5,312};
    const double down[]={0,0,-1};
    assert(musa_core_mesh_bvh_pick_wcs(b,&shear,sh_origin,down,15,
                                       &world,&stats)==1);
    near(world.distance,12,1e-10);
    near(world.wcs_x,101.25,1e-10);
    near(world.wcs_y,201.5,1e-10);
    near(world.wcs_z,300,1e-10);
    near(world.barycentric_u,.25,1e-10);
    near(world.barycentric_v,.25,1e-10);

    /* Rotation through 90 degrees (XZ vertical face), direction along X. */
    const MusaCoreAffine3 turn={{0,0,1,5,
                                  0,1,0,4,
                                  -1,0,0,2}};
    const double sideways_origin[]={9,4.5,1.5};
    const double west[]={-1,0,0};
    assert(musa_core_mesh_bvh_pick_wcs(b,&turn,sideways_origin,west,10,
                                       &world,NULL)==1);
    near(world.distance,4,1e-10);
    near(world.wcs_x,5,1e-10);

    /* Invalid transforms and rays cannot contaminate previous selection. */
    MusaCoreAffine3 singular={{1,0,0,0, 1,0,0,0, 0,0,1,0}};
    MusaCoreAffine3 invalid=id;invalid.m[0]=NAN;
    const double zero[]={0,0,0};
    world.distance=4242;world.triangle_index=333;
    stats.tested_faces=777;
    assert(musa_core_mesh_bvh_pick_wcs(b,&singular,o,d,10,
                                       &world,&stats)==-1);
    assert(musa_core_mesh_bvh_pick_wcs(b,&invalid,o,d,10,
                                       &world,&stats)==-1);
    assert(musa_core_mesh_bvh_pick_wcs(b,&id,o,zero,10,
                                       &world,&stats)==-1);
    assert(musa_core_mesh_bvh_pick_wcs(b,&id,o,d,-1,
                                       &world,&stats)==-1);
    assert(musa_core_mesh_bvh_pick_wcs(NULL,&id,o,d,10,
                                       &world,&stats)==-1);
    assert(world.distance==4242&&world.triangle_index==333);
    assert(stats.tested_faces==777);
    musa_core_mesh_bvh_destroy(b);
    puts("MusaCAD portable BVH WCS nested-block picking PASS");
    return 0;
}
