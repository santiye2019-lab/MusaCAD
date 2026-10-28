/* SPDX-License-Identifier: MIT
 * Independently authored analytic 2D bounds; no DWG decoder dependency.
 */
#ifndef MUSA_SCENE2D_GEOMETRY_H
#define MUSA_SCENE2D_GEOMETRY_H
#include "musa_scene2d_cull.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Conservative local-space bounds for a decoded LINE, CIRCLE or ARC.
 * ARC angles are radians, counterclockwise from +X. Equal angles describe
 * a zero-sweep arc; an absolute sweep >= 2*pi is a full circle.
 * Inputs must be finite; circles and arcs require positive radius.
 * On failure *out remains unchanged. Does not decode DWG records.
 */
int musa_scene2d_line_bounds(double x1, double y1, double x2, double y2,
                             MusaScene2dBox *out);
int musa_scene2d_circle_bounds(double cx, double cy, double radius,
                               MusaScene2dBox *out);
int musa_scene2d_arc_bounds(double cx, double cy, double radius,
                            double start_rad, double end_rad,
                            MusaScene2dBox *out);
#ifdef __cplusplus
}
#endif
#endif
