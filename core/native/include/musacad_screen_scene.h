/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_SCREEN_SCENE_H
#define MUSACAD_SCREEN_SCENE_H

#include "musacad_bvh_scene.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Portable Android/iPhone/desktop 3D selection entry points.
 *
 * Coordinates are PHYSICAL viewport pixels, origin at top-left; the UI must
 * subtract the viewport origin and convert logical/DIP units before calling.
 * world_to_clip is column-major double[16], with the same camera used for
 * rendering. depth_zero_to_one=0 for OpenGL; 1 for Metal/Direct3D.
 *
 * Screen click -> world ray -> nearest visible exact BVH triangle.
 * 1 hit, 0 miss, -1 invalid input/transform. On 0/-1, hit is unchanged.
 * Stats follows musa_core_bvh_scene_pick_wcs semantics.
 */
int musa_core_screen_scene_pick(
    const MusaCoreBVHInstance *instances, size_t instance_count,
    const double world_to_clip[16], double viewport_width,
    double viewport_height, double screen_x, double screen_y,
    int depth_zero_to_one, double max_world_distance,
    MusaCoreBVHSceneHit *hit, MusaCoreBVHSceneStats *stats);

/*
 * Screen rectangle -> DWG-like mesh instance indices, in stable input order.
 * crossing=0 WINDOW: WCS instance AABB must be completely inside.
 * crossing=1 CROSSING: WCS instance AABB need only intersect.
 * The latter is a BROAD PHASE: it may include faces outside the rectangle
 * and cannot imply visibility through solids/occlusion, precise entity
 * clipping, or complete AutoCAD WINDOW/CROSSING behaviour.
 *
 * Read-only caller-owned BVHs, instances and transforms must remain stable
 * during both query passes. Hidden objects are excluded; malformed visible
 * objects fail closed even if they would be outside the rectangle.
 *
 * out_indices=NULL, capacity=0 performs COUNT ONLY. Otherwise capacity must
 * accommodate ALL matches, or -1 is returned without writing either output.
 * No partial selection is ever published. The count is capped at 65536.
 * 1 nonempty, 0 empty, -1 invalid scene/viewport/capacity/overflow.
 */
int musa_core_screen_scene_candidates(
    const MusaCoreBVHInstance *instances, size_t instance_count,
    const double world_to_clip[16], double viewport_width,
    double viewport_height, double left, double top,
    double right, double bottom, int depth_zero_to_one,
    int crossing, size_t *out_indices, size_t capacity,
    size_t *out_count);

#ifdef __cplusplus
}
#endif
#endif
