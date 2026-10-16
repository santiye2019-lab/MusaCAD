/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_screen_scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near(double actual, double expected) {
    assert(isfinite(actual) && fabs(actual - expected) < 1e-10);
}

int main(void) {
    const float xyz[] = {-.2f,-.2f,0, .2f,-.2f,0, 0,.2f,0};
    const int faces[] = {0,1,2};
    MusaCoreMeshBVH *mesh = NULL;
    assert(musa_core_mesh_bvh_build_f32(xyz, 3, faces, 1, &mesh) && mesh);

    const double identity[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    MusaCoreAffine3 central = musa_core_affine3_identity();
    MusaCoreAffine3 shifted = central;
    shifted.m[3] = .7;
    MusaCoreBVHInstance scene[] = {
        {mesh, central, 101, 1},
        {mesh, shifted, 202, 1},
        {NULL, central, 303, 0} /* hidden unloaded INSERT */
    };
    size_t picks[] = {999,999,999};
    size_t n = 444;

    /* Contained WINDOW: only central object; candidate order == scene order. */
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,60,60,140,140,0,0,picks,3,&n) == 1);
    assert(n == 1 && picks[0] == 0);

    /* Crossing sees BOTH partially overlapping whole-object AABBs. */
    picks[0] = picks[1] = 999;
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,112,60,175,140,0,1,picks,3,&n) == 1);
    assert(n == 2 && picks[0] == 0 && picks[1] == 1);
    /* The same rectangle contains neither object completely. */
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,112,60,175,140,0,0,picks,3,&n) == 0);
    assert(n == 0);

    /* A separate full window matches just the shifted DWG-like block. */
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,140,60,195,140,0,0,picks,3,&n) == 1);
    assert(n == 1 && picks[0] == 1);

    /* First pass may COUNT without writing unallocated output. */
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,112,60,175,140,0,1,NULL,0,&n) == 1);
    assert(n == 2);
    picks[0] = picks[1] = 999; n = 777;
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,112,60,175,140,0,1,picks,1,&n) == -1);
    assert(n == 777 && picks[0] == 999 && picks[1] == 999);

    /* Actual screen click: same physical touch coordinates produce WCS ray
     * and then a precise triangle hit (not merely a box candidate).
     */
    MusaCoreBVHSceneHit hit = {0};
    MusaCoreBVHSceneStats diag = {0};
    assert(musa_core_screen_scene_pick(scene, 3, identity, 200,200,
                                       100,100,0,2,&hit,&diag) == 1);
    assert(hit.object_id == 101 && hit.instance_index == 0);
    near(hit.face.wcs_z, 0);
    near(hit.face.distance, 1);
    assert(musa_core_screen_scene_pick(scene, 3, identity, 200,200,
                                       170,100,0,2,&hit,&diag) == 1);
    assert(hit.object_id == 202 && hit.instance_index == 1);
    near(hit.face.wcs_x, .7);
    hit.object_id = 888;
    assert(musa_core_screen_scene_pick(scene, 3, identity, 200,200,
                                       190,15,0,2,&hit,&diag) == 0);
    assert(hit.object_id == 888);

    /* Hidden objects do not appear even when exactly under the touch. */
    scene[0].visible = 0;
    assert(musa_core_screen_scene_pick(scene, 3, identity, 200,200,
                                       100,100,0,2,&hit,&diag) == 0);
    assert(hit.object_id == 888);
    scene[0].visible = 1;

    /* Invalid visible INSERT fails CLOSED even if outside the selection.
     * Never silently drop malformed objects or publish partial results.
     */
    scene[1].local_to_wcs.m[0] = 0;
    scene[1].local_to_wcs.m[5] = 0;
    scene[1].local_to_wcs.m[10] = 0;
    picks[0] = 999; n = 777;
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,60,60,140,140,0,0,picks,3,&n) == -1);
    assert(picks[0] == 999 && n == 777);
    assert(musa_core_screen_scene_pick(scene, 3, identity, 200,200,
                                       100,100,0,2,&hit,&diag) == -1);
    assert(hit.object_id == 888);
    scene[1].local_to_wcs = shifted;

    /* Perspective viewport, correct top-left touch Y, and near/far clipping. */
    const double perspective[16] = {
        1,0,0,0, 0,1,0,0,
        0,0,-11./9.,-1., 0,0,-20./9.,0
    };
    scene[0].local_to_wcs.m[11] = -3;
    scene[1].visible = 0;
    assert(musa_core_screen_scene_candidates(scene, 3, perspective,
           200,200,50,50,150,150,0,0,picks,3,&n) == 1);
    assert(n == 1 && picks[0] == 0);
    assert(musa_core_screen_scene_pick(scene, 3, perspective, 200,200,
                                       100,100,0,9,&hit,&diag) == 1);
    assert(hit.object_id == 101);
    near(hit.face.distance, 2);

    /* Direct3D/Metal clip depth 0..1, with a valid positive near depth. */
    scene[0].local_to_wcs = central;
    scene[0].local_to_wcs.m[11] = .25;
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,60,60,140,140,1,0,picks,3,&n) == 1);
    assert(n == 1 && picks[0] == 0);
    assert(musa_core_screen_scene_pick(scene, 3, identity, 200,200,
                                       100,100,1,2,&hit,&diag) == 1);
    near(hit.face.distance, .25);

    /* Big DWG WCS offsets retain precision through matrix and INSERT. */
    const double wcs_camera[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0,
        -1000000,2000000,-3000000,1
    };
    scene[0].local_to_wcs = central;
    scene[0].local_to_wcs.m[3] = 1000000;
    scene[0].local_to_wcs.m[7] = -2000000;
    scene[0].local_to_wcs.m[11] = 3000000;
    assert(musa_core_screen_scene_candidates(scene, 3, wcs_camera,
           200,200,60,60,140,140,0,0,picks,3,&n) == 1);
    assert(n == 1 && picks[0] == 0);

    /* Invalid rectangles, matrices, scene count and capacity are rejected. */
    const double singular[16] = {0};
    picks[0] = 999; n = 777;
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,150,60,140,140,0,0,picks,3,&n) == -1);
    assert(musa_core_screen_scene_candidates(scene, 3, singular,
           200,200,60,60,140,140,0,1,picks,3,&n) == -1);
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,60,60,140,140,0,2,picks,3,&n) == -1);
    assert(musa_core_screen_scene_candidates(scene, 3, identity,
           200,200,60,60,140,140,0,0,NULL,2,&n) == -1);
    assert(musa_core_screen_scene_candidates(NULL, 65537, identity,
           200,200,60,60,140,140,0,0,picks,3,&n) == -1);
    assert(picks[0] == 999 && n == 777);
    hit.object_id = 123;
    assert(musa_core_screen_scene_pick(scene, 3, singular, 200,200,
                                       100,100,0,2,&hit,&diag) == -1);
    assert(hit.object_id == 123);

    /*
     * Canvas legacy front-camera fixture, radius=1, yaw=pitch=0,
     * zoom=1, viewport=200x200. This is the exact world-to-clip layout
     * produced by Cad3dLegacyCameraBridge: X is screen-right, Z is
     * screen-up, and increasing Y travels away from the viewer.
     * C ray unprojection must hit the same central visible triangle.
     */
    const float front_xyz[] = {-.2f,0,-.2f, .2f,0,-.2f, 0,0,.2f};
    MusaCoreMeshBVH *front_mesh = NULL;
    assert(musa_core_mesh_bvh_build_f32(front_xyz, 3, faces, 1,
                                         &front_mesh) && front_mesh);
    const double canvas_front[16] = {
        1./1.3,0,0,0,
        0,0,.8,.4,
        0,1./1.3,0,0,
        0,0,0,1
    };
    MusaCoreBVHInstance front_scene[] = {
        {front_mesh, central, 4242, 1}
    };
    assert(musa_core_screen_scene_pick(front_scene, 1, canvas_front,
                                       200,200,100,100,0,10,&hit,&diag) == 1);
    assert(hit.object_id == 4242 && hit.instance_index == 0);
    near(hit.face.wcs_x, 0);
    near(hit.face.wcs_y, 0);
    near(hit.face.wcs_z, 0);
    assert(musa_core_screen_scene_candidates(front_scene, 1, canvas_front,
           200,200,70,70,130,130,0,0,picks,3,&n) == 1);
    assert(n == 1 && picks[0] == 0);
    musa_core_mesh_bvh_destroy(front_mesh);

    musa_core_mesh_bvh_destroy(mesh);
    puts("MusaCAD portable Android/desktop/iPhone screen-scene click and rectangle PASS");
    return 0;
}
