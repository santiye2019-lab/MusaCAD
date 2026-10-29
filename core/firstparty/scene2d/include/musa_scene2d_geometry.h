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
/* Full planar ELLIPSE bounds from a major-axis vector (ax,ay) and a
 * positive minor/major ratio <= 1. This is a full ellipse, NOT a partial
 * elliptic arc. Angles, bulges, extrusion and DWG decoding are outside
 * this helper. All inputs must be finite; invalid results preserve *out. */
int musa_scene2d_ellipse_bounds(double cx, double cy,
                                double ax, double ay, double minor_ratio,
                                MusaScene2dBox *out);
/* Conservative bounds for a decoded planar ELLIPSE arc, parameterized by
 * p(t)=(cx,cy)+(ax,ay)*cos(t)+ratio*(-ay,ax)*sin(t).
 * The sweep is counterclockwise; end before start wraps through 2*pi.
 * Equal parameters describe a point; absolute span >= 2*pi is full.
 * No OCS/extrusion or DWG byte decoding is performed. On failure the
 * output is unchanged. */
int musa_scene2d_ellipse_arc_bounds(double cx, double cy,
                                    double ax, double ay, double minor_ratio,
                                    double start_rad, double end_rad,
                                    MusaScene2dBox *out);
#ifdef __cplusplus
}
#endif
#endif
