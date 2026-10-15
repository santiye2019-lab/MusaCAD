/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_BVH_SCENE_H
#define MUSACAD_BVH_SCENE_H
#include "musacad_bvh_wcs.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * A read-only list of DWG-like 3D block/mesh instances. The same immutable
 * BVH may be referenced by multiple transforms without copying geometry.
 * Caller owns meshes and the instance array throughout the query; do not
 * destroy a BVH while another thread uses it.
 *
 * Visibility is explicitly 0/1: hidden instances are never inspected, even
 * if their mesh pointer is null. All visible instances must have a mesh and
 * invertible finite WCS matrix; malformed visible input fails closed.
 * Object-level visibility is the caller's resolved layer/block visibility.
 * This API cannot infer CAD clip planes, scene depth buffers or XREF state.
 */
typedef struct MusaCoreBVHInstance {
    const MusaCoreMeshBVH *mesh;
    MusaCoreAffine3 local_to_wcs;
    size_t object_id;
    int visible;
} MusaCoreBVHInstance;
typedef struct MusaCoreBVHSceneHit {
    size_t instance_index;
    size_t object_id;
    MusaCoreMeshRayHit face;
} MusaCoreBVHSceneHit;
typedef struct MusaCoreBVHSceneStats {
    size_t visible_instances,visited_instances;
    size_t visited_nodes,rejected_nodes,exact_triangle_tests;
} MusaCoreBVHSceneStats;
/*
 * Return 1 hit, 0 miss, -1 invalid input or arithmetic overflow.
 * A hit is the closest positive-distance visible object in WCS drawing units.
 * Ties retain the first instance in input order. *out is never modified on
 * 0/-1; *stats is modified only when the entire query succeeds (0/1).
 * No AutoCAD 2026 or ACIS/B-Rep parity claims are implied.
 */
int musa_core_bvh_scene_pick_wcs(
    const MusaCoreBVHInstance *instances,size_t instance_count,
    const double ray_origin_wcs[3],const double ray_direction_wcs[3],
    double max_world_distance,MusaCoreBVHSceneHit *out,
    MusaCoreBVHSceneStats *stats);
#ifdef __cplusplus
}
#endif
#endif
