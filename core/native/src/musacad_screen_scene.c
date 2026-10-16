/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_screen_scene.h"
#include "musacad_camera_ray.h"
#include "musacad_screen_frustum.h"
#include <math.h>

/* Mirrors the scene BVH's bounded instance contract. */
#define MUSA_SCREEN_SCENE_MAX_INSTANCES 65536u

int musa_core_screen_scene_pick(
    const MusaCoreBVHInstance *instances, size_t count,
    const double world_to_clip[16], double width, double height,
    double x, double y, int z01, double max_distance,
    MusaCoreBVHSceneHit *hit, MusaCoreBVHSceneStats *stats) {
    if (!hit) return -1;
    double origin[3], direction[3];
    if (!musa_core_camera_screen_ray(world_to_clip, width, height, x, y,
                                     z01, origin, direction))
        return -1;
    return musa_core_bvh_scene_pick_wcs(instances, count, origin, direction,
                                        max_distance, hit, stats);
}

/* Resolve a local immutable mesh's bounds after DWG INSERT-style transforms.
 * Transform all eight local AABB corners: mirrors, shear and nonuniform
 * scaling must NEVER be approximated by transforming just min/max.
 */
static int instance_wcs_bounds(const MusaCoreBVHInstance *inst,
                               double mn[3], double mx[3]) {
    double local_min[3], local_max[3];
    int exists = musa_core_mesh_bvh_bounds(inst->mesh, local_min, local_max);
    if (exists <= 0) return exists;
    for (unsigned corner = 0; corner < 8; ++corner) {
        double point[3], mapped[3];
        for (int axis = 0; axis < 3; ++axis)
            point[axis] = (corner & (1u << axis))
                        ? local_max[axis] : local_min[axis];
        if (!musa_core_affine3_map_point(&inst->local_to_wcs,
                                          point[0], point[1], point[2], mapped))
            return -1;
        for (int axis = 0; axis < 3; ++axis) {
            if (corner == 0) mn[axis] = mx[axis] = mapped[axis];
            else {
                if (mapped[axis] < mn[axis]) mn[axis] = mapped[axis];
                if (mapped[axis] > mx[axis]) mx[axis] = mapped[axis];
            }
        }
    }
    return 1;
}

/* -1 invalid visible instance, 0 not selected, 1 candidate selected. */
static int classify_instance(const MusaCoreBVHInstance *inst,
                              const MusaCoreScreenFrustum *frustum,
                              int crossing) {
    if (inst->visible != 0 && inst->visible != 1) return -1;
    if (!inst->visible) return 0;
    if (!inst->mesh || !musa_core_affine3_finite(&inst->local_to_wcs))
        return -1;

    /* Do not conceal a malformed visible block behind frustum culling. */
    MusaCoreAffine3 inverse;
    if (!musa_core_affine3_inverse(&inst->local_to_wcs, &inverse))
        return -1;

    double wcs_min[3], wcs_max[3];
    int exists = instance_wcs_bounds(inst, wcs_min, wcs_max);
    if (exists <= 0) return exists; /* empty mesh or invalid geometry */

    int relation = musa_core_screen_frustum_classify_aabb(frustum,
                                                           wcs_min, wcs_max);
    if (relation < 0) return -1;
    return (crossing ? relation != 0 : relation == 2) ? 1 : 0;
}

int musa_core_screen_scene_candidates(
    const MusaCoreBVHInstance *instances, size_t count,
    const double world_to_clip[16], double width, double height,
    double left, double top, double right, double bottom, int z01,
    int crossing, size_t *indices, size_t capacity, size_t *out_count) {
    if (!out_count || count > MUSA_SCREEN_SCENE_MAX_INSTANCES ||
        (count && !instances) ||
        (crossing != 0 && crossing != 1) ||
        (!indices && capacity != 0)) return -1;

    MusaCoreScreenFrustum frustum;
    if (!musa_core_screen_frustum_build(world_to_clip, width, height,
                                         left, top, right, bottom,
                                         z01, &frustum)) return -1;

    size_t needed = 0;
    /* Pass one validates the WHOLE visible scene before anything is written. */
    for (size_t i = 0; i < count; ++i) {
        int accepted = classify_instance(&instances[i], &frustum, crossing);
        if (accepted < 0) return -1;
        if (accepted) ++needed; /* bounded by count <= 65536 */
    }
    if (indices && needed > capacity) return -1;

    /* Count-only mode is important for bounded Android allocations. */
    if (indices) {
        size_t written = 0;
        for (size_t i = 0; i < count; ++i) {
            int accepted = classify_instance(&instances[i], &frustum, crossing);
            if (accepted < 0) return -1; /* caller must keep scene immutable */
            if (accepted) indices[written++] = i;
        }
    }
    *out_count = needed;
    return needed ? 1 : 0;
}
