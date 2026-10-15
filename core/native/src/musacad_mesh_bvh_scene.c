/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bvh_scene.h"
#include <math.h>
#include <stdint.h>

/* A small, bounded and deterministic scene-level broad phase.
 * BVH face acceleration happens inside each shared mesh object.
 */
#define MUSA_SCENE_MAX_INSTANCES 65536u
static int checked_accumulate(size_t *dst,size_t add) {
    if(*dst>SIZE_MAX-add)return 0;
    *dst+=add;return 1;
}
int musa_core_bvh_scene_pick_wcs(
    const MusaCoreBVHInstance *instances,size_t count,
    const double origin[3],const double direction[3],
    double max_distance,MusaCoreBVHSceneHit *out,
    MusaCoreBVHSceneStats *stats) {
    if(!out||!origin||!direction||!isfinite(max_distance)||
       max_distance<0.||count>MUSA_SCENE_MAX_INSTANCES||
       (count&&!instances))return -1;
    for(int k=0;k<3;k++)
        if(!isfinite(origin[k])||!isfinite(direction[k]))return -1;
    double magnitude=fmax(fabs(direction[0]),
                     fmax(fabs(direction[1]),fabs(direction[2])));
    if(magnitude==0.)return -1;
    MusaCoreBVHSceneStats totals={0};
    MusaCoreBVHSceneHit closest={0};
    int found=0;
    for(size_t i=0;i<count;i++) {
        const MusaCoreBVHInstance *inst=&instances[i];
        if(inst->visible!=0&&inst->visible!=1)return -1;
        if(!inst->visible)continue;
        if(!inst->mesh||!musa_core_affine3_finite(&inst->local_to_wcs))
            return -1;
        if(!checked_accumulate(&totals.visible_instances,1)||
           !checked_accumulate(&totals.visited_instances,1))return -1;
        MusaCoreMeshRayHit face={0};
        MusaCoreMeshBVHStats s={0};
        double limit=found?fmin(max_distance,closest.face.distance):
                           max_distance;
        int rc=musa_core_mesh_bvh_pick_wcs(inst->mesh,&inst->local_to_wcs,
                                           origin,direction,limit,&face,&s);
        if(rc<0)return -1;
        if(!checked_accumulate(&totals.visited_nodes,s.visited_nodes)||
           !checked_accumulate(&totals.rejected_nodes,s.rejected_nodes)||
           !checked_accumulate(&totals.exact_triangle_tests,
                               s.tested_faces))return -1;
        if(rc==1&&(!found||face.distance<closest.face.distance)){
            closest.instance_index=i;closest.object_id=inst->object_id;
            closest.face=face;found=1;
        }
    }
    if(stats)*stats=totals;
    if(!found)return 0;
    *out=closest;return 1;
}
