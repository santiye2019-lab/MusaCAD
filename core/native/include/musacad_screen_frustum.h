/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_SCREEN_FRUSTUM_H
#define MUSACAD_SCREEN_FRUSTUM_H
/*
 * C11 screen window/crossing coarse selection for 3D CAD.
 * All matrices are column-major world-to-clip 4x4 (same as camera_ray).
 * Screen rectangle uses top-left viewport pixel-edge coordinates.
 * clip_depth_zero_one=0 OpenGL -1..1 NDC depth; =1 Metal/D3D 0..1.
 *
 * This classifies a WORLD-SPACE axis-aligned bounding box against the
 * selected screen rectangle extruded between the camera near/far planes.
 * OUTSIDE(0) means the actual object cannot intersect the rectangle;
 * INTERSECTS(1) conservatively includes potential false positives;
 * INSIDE(2) means the full AABB is within all frustum halfspaces.
 * INVALID(-1) indicates invalid inputs/nonfinite numeric intermediate.
 *
 * These are AABB candidate tests, not exact mesh polygon clipping,
 * pixel hit test, hidden-line selection or AutoCAD 2026 parity.
 */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaCoreScreenFrustum {
    double planes[6][4]; /* n_xyz and D; inside: dot(n,p)+D >= 0 */
} MusaCoreScreenFrustum;
int musa_core_screen_frustum_build(
    const double world_to_clip_colmajor[16],
    double viewport_width,double viewport_height,
    double left,double top,double right,double bottom,
    int clip_depth_zero_one,MusaCoreScreenFrustum *out);
int musa_core_screen_frustum_classify_aabb(
    const MusaCoreScreenFrustum *frustum,
    const double wcs_min[3],const double wcs_max[3]);
#ifdef __cplusplus
}
#endif
#endif
