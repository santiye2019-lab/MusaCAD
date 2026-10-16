/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CAMERA_RAY_H
#define MUSACAD_CAMERA_RAY_H
/*
 * Shared 3D CAD camera click/touch -> WCS selection ray, portable C11.
 *
 * view_projection[16] is column-major (OpenGL-compatible), and transforms
 * WORLD coordinates to homogeneous CLIP coordinates. No platform UI API.
 *
 * screen_x/screen_y use top-left pixel-edge coordinates. For example,
 * (viewport_width/2,viewport_height/2) is the exact center. UI adapters
 * must apply touch/screen density and viewport offsets themselves.
 *
 * depth_zero_to_one = 0 for OpenGL/Vulkan-style -1..1 NDC Z,
 * depth_zero_to_one = 1 for 0..1 NDC Z (Direct3D/Metal/Vulkan).
 * The API never assumes whether the projection is ortho or perspective.
 *
 * Output ray_origin is the world near-clip plane point, not camera eye.
 * Output normalized ray_direction points toward the unprojected far point.
 * This convention correctly covers orthographic and perspective cameras.
 *
 * Return 1 on success, 0 on invalid/singular/nonfinite transform, viewport
 * or unprojectable homogeneous coordinates. All output arrays remain
 * unchanged on failure. Infinite far planes require a different API.
 *
 * Geometry math is shared by Android, Windows and iPhone; it is not yet
 * wired to the existing Android gesture/view/selection UI.
 */
#ifdef __cplusplus
extern "C" {
#endif
int musa_core_camera_screen_ray(
    const double view_projection_colmajor[16],
    double viewport_width, double viewport_height,
    double screen_x,double screen_y,int depth_zero_to_one,
    double out_ray_origin_wcs[3],double out_ray_direction_wcs[3]);
#ifdef __cplusplus
}
#endif
#endif
