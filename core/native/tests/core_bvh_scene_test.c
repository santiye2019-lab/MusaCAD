/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bvh_scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(double x,double y,double eps) {
    assert(isfinite(x)&&fabs(x-y)<=eps);
}
int main(void){
    const float p[]={0,0,0,2,0,0,0,2,0};
    const int faces[]={0,1,2};
    MusaCoreMeshBVH *b=NULL;
    assert(musa_core_mesh_bvh_build_f32(p,3,faces,1,&b)&&b);
    MusaCoreAffine3 id=musa_core_affine3_identity();
    MusaCoreAffine3 near_block=id;near_block.m[11]=4;
    MusaCoreAffine3 far_block=id;far_block.m[11]=-2;
    MusaCoreAffine3 mirror=id;mirror.m[0]=-2;mirror.m[3]=10;
    MusaCoreBVHInstance inst[]={
        {b,id,101,1},
        {b,near_block,202,1},
        {b,far_block,303,1},
        {b,mirror,404,0}
    };
    const double origin[]={.5,.5,10},dir[]={0,0,-7};
    MusaCoreBVHSceneHit hit={0};
    MusaCoreBVHSceneStats diag={0};
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,&hit,&diag)==1);
    assert(hit.instance_index==1&&hit.object_id==202);
    assert(hit.face.triangle_index==0&&hit.face.front_facing);
    near(hit.face.distance,6.,1e-12);
    near(hit.face.wcs_z,4.,1e-12);
    assert(diag.visible_instances==3&&diag.visited_instances==3);
    assert(diag.exact_triangle_tests>0&&diag.exact_triangle_tests<=3);
    /* Hide nearest instance without deleting the indexed mesh. */
    inst[1].visible=0;
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,&hit,&diag)==1);
    assert(hit.instance_index==0&&hit.object_id==101);
    near(hit.face.distance,10.,1e-12);
    /* Tie between two identical block INSERTs picks stable first object. */
    inst[1].visible=1;inst[1].local_to_wcs=id;
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,&hit,NULL)==1);
    assert(hit.instance_index==0);
    inst[1].local_to_wcs=near_block;
    /* The same mesh mirrored elsewhere in WCS has the opposite facing. */
    inst[0].visible=0;inst[1].visible=0;inst[2].visible=0;
    inst[3].visible=1;
    const double mirror_origin[]={9,.5,10};
    assert(musa_core_bvh_scene_pick_wcs(inst,4,mirror_origin,dir,20,
                                        &hit,&diag)==1);
    assert(hit.instance_index==3&&hit.object_id==404);
    assert(!hit.face.front_facing);
    near(hit.face.wcs_x,9,1e-12);
    near(hit.face.wcs_z,0,1e-12);
    assert(diag.visible_instances==1);
    /* Valid miss must not fabricate a hit; diagnostic counts may change. */
    hit.object_id=888;hit.instance_index=999;
    const double miss[]={40,40,10};
    assert(musa_core_bvh_scene_pick_wcs(inst,4,miss,dir,20,&hit,&diag)==0);
    assert(hit.object_id==888&&hit.instance_index==999);
    assert(diag.visible_instances==1);
    /* Fail CLOSED if later visible instance has a singular transform.
     * Earlier legitimate hits must not be published on a malformed scene.
     */
    inst[0].visible=1;inst[1].visible=1;
    inst[1].local_to_wcs.m[0]=0;
    inst[1].local_to_wcs.m[5]=0;
    inst[1].local_to_wcs.m[10]=0;
    diag.visible_instances=777;
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,
                                        &hit,&diag)==-1);
    assert(hit.object_id==888&&hit.instance_index==999);
    assert(diag.visible_instances==777);
    /* Hidden invalid input isn't pickable or required to be loaded. */
    inst[1].visible=0;inst[1].mesh=NULL;
    inst[3].visible=0;
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,
                                        &hit,&diag)==1);
    assert(hit.object_id==101);
    /* Explicit limits, empty scenes, invalid visibility and ray. */
    inst[0].visible=2;
    hit.object_id=111;diag.visible_instances=555;
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,
                                        &hit,&diag)==-1);
    assert(hit.object_id==111&&diag.visible_instances==555);
    const double zero[]={0,0,0};
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,zero,20,
                                        &hit,&diag)==-1);
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,-1,
                                        &hit,&diag)==-1);
    assert(musa_core_bvh_scene_pick_wcs(NULL,65537,origin,dir,20,
                                        &hit,&diag)==-1);
    assert(musa_core_bvh_scene_pick_wcs(NULL,0,origin,dir,20,
                                        &hit,&diag)==0);
    assert(diag.visible_instances==0);
    assert(musa_core_bvh_scene_pick_wcs(inst,4,origin,dir,20,
                                        NULL,&diag)==-1);
    musa_core_mesh_bvh_destroy(b);
    puts("MusaCAD cross-platform 3D visible scene BLOCK hit + fail-closed PASS");
    return 0;
}
