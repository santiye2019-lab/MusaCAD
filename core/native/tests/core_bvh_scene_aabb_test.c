/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bvh_scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(double a,double b){assert(isfinite(a)&&fabs(a-b)<1e-9);}
int main(void) {
    const float triangle[]={0,0,0,2,0,0,0,2,0};
    const int faces[]={0,1,2};
    MusaCoreMeshBVH *mesh=NULL;
    assert(musa_core_mesh_bvh_build_f32(triangle,3,faces,1,&mesh));
    double lmn[3]={0},lmx[3]={0};
    assert(musa_core_mesh_bvh_bounds(mesh,lmn,lmx)==1);
    near(lmn[0],0);near(lmn[1],0);near(lmn[2],0);
    near(lmx[0],2);near(lmx[1],2);near(lmx[2],0);
    MusaCoreBVHInstance scene[34]={0};
    MusaCoreAffine3 identity=musa_core_affine3_identity();
    for(size_t i=0;i<34;i++){
        scene[i].mesh=mesh;
        scene[i].local_to_wcs=identity;
        scene[i].object_id=100+i;
        scene[i].visible=1;
        scene[i].local_to_wcs.m[3]=1000.+(double)i*20.;
    }
    /* A far-off group must NOT run expensive mesh pick per instance. */
    scene[32].local_to_wcs.m[3]=0.;
    scene[32].local_to_wcs.m[11]=2.;
    scene[33].local_to_wcs.m[3]=0.;
    const double origin[]={.5,.5,10},direction[]={0,0,-10};
    MusaCoreBVHSceneHit result={0};
    MusaCoreBVHSceneStats stats={0};
    assert(musa_core_bvh_scene_pick_wcs(scene,34,origin,direction,
                                        15.,&result,&stats)==1);
    assert(result.object_id==132&&result.instance_index==32);
    near(result.face.distance,8.);
    assert(stats.visible_instances==34&&stats.visited_instances==34);
    assert(stats.bounds_rejected_instances>=32);
    assert(stats.exact_triangle_tests<=2);
    /* Candidate max WCS distance also prunes face behind the first. */
    const double miss[]={-100,-100,10};
    result.object_id=9876;
    assert(musa_core_bvh_scene_pick_wcs(scene,34,miss,direction,15.,
                                        &result,&stats)==0);
    assert(result.object_id==9876);
    assert(stats.bounds_rejected_instances==34);
    assert(stats.exact_triangle_tests==0);
    /* Shear/negative X mirror maps all local AABB corners correctly. */
    scene[32].local_to_wcs.m[0]=-2;
    scene[32].local_to_wcs.m[1]=.5;
    scene[32].local_to_wcs.m[3]=10;
    const double mirrored[]={9.25,.5,10};
    assert(musa_core_bvh_scene_pick_wcs(scene,34,mirrored,direction,
                                        15.,&result,&stats)==1);
    assert(result.object_id==132);
    assert(!result.face.front_facing);
    near(result.face.wcs_x,9.25);
    near(result.face.distance,8);
    /* A singular later visible instance must NOT be hidden by AABB pruning.
     * Previously selected result must stay untouched on invalid input.
     */
    scene[31].local_to_wcs.m[0]=0;
    scene[31].local_to_wcs.m[5]=0;
    scene[31].local_to_wcs.m[10]=0;
    result.object_id=12345;stats.bounds_rejected_instances=999;
    assert(musa_core_bvh_scene_pick_wcs(scene,34,mirrored,direction,
                                        15.,&result,&stats)==-1);
    assert(result.object_id==12345&&stats.bounds_rejected_instances==999);
    musa_core_mesh_bvh_destroy(mesh);
    /* Empty BVH: no face bounds, no fabricated AABB. */
    MusaCoreMeshBVH *empty=NULL;
    assert(musa_core_mesh_bvh_build_f32(NULL,0,NULL,0,&empty));
    lmn[0]=123;lmx[0]=456;
    assert(musa_core_mesh_bvh_bounds(empty,lmn,lmx)==0);
    assert(lmn[0]==123&&lmx[0]==456);
    assert(musa_core_mesh_bvh_bounds(NULL,lmn,lmx)==-1);
    musa_core_mesh_bvh_destroy(empty);
    puts("MusaCAD shared scene WCS AABB coarse rejection PASS");
    return 0;
}
