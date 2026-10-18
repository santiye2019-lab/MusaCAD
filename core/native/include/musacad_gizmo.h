/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_GIZMO_H
#define MUSACAD_GIZMO_H
/* One portable implementation of the previously Android-only 3D gizmo
 * projection, handle selection and drag distance. The Android JNI passes
 * numbers only; Windows and iOS use these exact C functions.
 */
#ifdef __cplusplus
extern "C" {
#endif
/* 1 success, 0 invalid. Output unchanged on failure. */
int musa_core_gizmo_axis_direction(const double axis_wcs[3],
                                   double yaw,double pitch,double out_xy[2]);
/* Return selected axis 0/1/2; -1 for no handle, invalid or hidden axis.
 * axes_xy = {x_screen_x, x_screen_y, y_x, y_y, z_x, z_y};
 * Nonfinite individual directions are skipped conservatively.
 */
int musa_core_gizmo_pick(double touch_x,double touch_y,
                         const double axes_xy[6],
                         double handle_pixels,double tolerance_pixels);
/* NaN when the screen-projected axis cannot safely be used. */
double musa_core_gizmo_drag_distance(double dx_pixels,double dy_pixels,
                                     double axis_x,double axis_y,
                                     double pixels_per_unit);
#ifdef __cplusplus
}
#endif
#endif
