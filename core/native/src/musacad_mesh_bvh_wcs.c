/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bvh_wcs.h"
#include <math.h>

/* Coordinate-space bridge: ALL triangle math remains in the shared BVH.
 * Mapping the WCS ray into object coordinates makes one immutable mesh BVH
 * reusable across many BLOCK/INSERT instances and platforms.
 */
int musa_core_mesh_bvh_pick_wcs(
    const MusaCoreMeshBVH *mesh,
    const MusaCoreAffine3 *object_to_wcs,
    const double origin_wcs[3],
    const double direction_wcs[3],
    double max_world_distance,
    MusaCoreMeshRayHit *out,
    MusaCoreMeshBVHStats *stats) {

    if(!mesh||!object_to_wcs||!origin_wcs||!direction_wcs||!out||
       !musa_core_affine3_finite(object_to_wcs)||
       !isfinite(max_world_distance)||max_world_distance<0.)return -1;

    for(int k=0;k<3;k++)
        if(!isfinite(origin_wcs[k])||!isfinite(direction_wcs[k]))return -1;

    /* Normalize WCS first to keep the max-distance contract independent
     * of the magnitude of the caller's direction vector. */
    const double scale=fmax(fabs(direction_wcs[0]),
                        fmax(fabs(direction_wcs[1]),fabs(direction_wcs[2])));
    if(scale==0.)return -1;
    double dir_wcs[3]={direction_wcs[0]/scale,
                       direction_wcs[1]/scale,
                       direction_wcs[2]/scale};
    const double norm_wcs=hypot(hypot(dir_wcs[0],dir_wcs[1]),dir_wcs[2]);
    if(!isfinite(norm_wcs)||norm_wcs==0.)return -1;
    for(int i=0;i<3;i++)dir_wcs[i]/=norm_wcs;

    MusaCoreAffine3 wcs_to_local;
    if(!musa_core_affine3_inverse(object_to_wcs,&wcs_to_local))
        return -1;

    double origin_local[3], local_dir[3];
    if(!musa_core_affine3_map_point(
            &wcs_to_local,origin_wcs[0],origin_wcs[1],origin_wcs[2],
            origin_local)||
       !musa_core_affine3_map_vector(
            &wcs_to_local,dir_wcs[0],dir_wcs[1],dir_wcs[2],
            local_dir))return -1;

    /* local_dir maps a unit WCS direction into local coordinates, with
     * length [local units / world unit]. BVH normalizes its input.
     */
    const double local_speed=hypot(hypot(local_dir[0],local_dir[1]),
                                   local_dir[2]);
    if(!isfinite(local_speed)||local_speed==0.)return -1;
    const double local_limit=max_world_distance*local_speed;
    if(!isfinite(local_limit))return -1;

    double determinant=0.;
    if(!musa_core_affine3_determinant(object_to_wcs,&determinant)||
       determinant==0.)return -1;

    MusaCoreMeshRayHit local_hit;
    MusaCoreMeshBVHStats counts={0};
    int result=musa_core_mesh_bvh_pick_stats(
        mesh,origin_local,local_dir,local_limit,&local_hit,&counts);
    if(result<0)return -1;
    if(result==0){
        if(stats)*stats=counts;
        return 0;
    }

    /* Convert selected local intersection back into drawing coordinates.
     * This is more precise than reconstructing the hit from rounded WCS
     * ray origin when the model is far from coordinate origin.
     */
    double hit_wcs[3];
    if(!musa_core_affine3_map_point(object_to_wcs,
          local_hit.wcs_x,local_hit.wcs_y,local_hit.wcs_z,hit_wcs))
        return -1;

    const double wcs_distance=local_hit.distance/local_speed;
    if(!isfinite(wcs_distance)||wcs_distance<0.||
       wcs_distance>max_world_distance)return -1;

    MusaCoreMeshRayHit candidate=local_hit;
    candidate.wcs_x=hit_wcs[0];
    candidate.wcs_y=hit_wcs[1];
    candidate.wcs_z=hit_wcs[2];
    candidate.distance=wcs_distance;
    if(determinant<0.)
        candidate.front_facing=!candidate.front_facing;

    *out=candidate;
    if(stats)*stats=counts;
    return 1;
}
