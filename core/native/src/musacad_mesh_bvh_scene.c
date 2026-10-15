/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bvh_scene.h"
#include "musacad_bounds3.h"
#include <math.h>
#include <stdint.h>

/* A small, bounded and deterministic scene-level broad phase.
 * BVH face acceleration happens inside each shared mesh object.
 */
#define MUSA_SCENE_MAX_INSTANCES 65536u
/*
 * An affine map sends an AABB's eight corners to a conservative WCS AABB,
 * including reflections, shear and nonuniform scales. Reject only when a
 * normalized WCS ray CANNOT meet that WCS box. This preserves the original
 * exact triangle test as the final source of selection truth.
 */
static int wcs_mesh_bounds(const MusaCoreMeshBVH *mesh,
                           const MusaCoreAffine3 *object_to_wcs,
                           double out_min[3],double out_max[3]) {
    double local_min[3],local_max[3];
    int exists=musa_core_mesh_bvh_bounds(mesh,local_min,local_max);
    if(exists<=0)return exists;
    double mn[3]={0},mx[3]={0};
    for(int corner=0;corner<8;corner++) {
        double src[3]={0},dst[3]={0};
        for(int a=0;a<3;a++)
            src[a]=(corner&(1<<a))?local_max[a]:local_min[a];
        if(!musa_core_affine3_map_point(object_to_wcs,
                                        src[0],src[1],src[2],dst))
            return -1;
        for(int a=0;a<3;a++){
            if(!corner){mn[a]=dst[a];mx[a]=dst[a];}
            else{
                if(dst[a]<mn[a])mn[a]=dst[a];
                if(dst[a]>mx[a])mx[a]=dst[a];
            }
        }
    }
    for(int a=0;a<3;a++){out_min[a]=mn[a];out_max[a]=mx[a];}
    return 1;
}
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
    double direction_unit[3]={direction[0]/magnitude,
                              direction[1]/magnitude,
                              direction[2]/magnitude};
    double ray_norm=hypot(hypot(direction_unit[0],direction_unit[1]),
                          direction_unit[2]);
    if(!isfinite(ray_norm)||ray_norm==0.)return -1;
    for(int k=0;k<3;k++)direction_unit[k]/=ray_norm;
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
        /* Keep the preexisting fail-closed contract: even an off-ray
         * VISIBLE object must have a valid invertible WCS transform.
         * Bounding culling must never conceal singular drawing geometry.
         */
        MusaCoreAffine3 inverse;
        if(!musa_core_affine3_inverse(&inst->local_to_wcs,&inverse))
            return -1;
        double wcs_min[3],wcs_max[3];
        int bounds=wcs_mesh_bounds(inst->mesh,&inst->local_to_wcs,
                                   wcs_min,wcs_max);
        if(bounds<0)return -1;
        if(bounds==0)continue;
        int maybe=musa_core_bounds3_ray_maybe(
            wcs_min,wcs_max,origin,direction_unit,limit);
        if(maybe<0)return -1;
        if(maybe==0){
            if(!checked_accumulate(&totals.bounds_rejected_instances,1))
                return -1;
            continue;
        }
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
