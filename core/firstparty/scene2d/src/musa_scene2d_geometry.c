/* SPDX-License-Identifier: MIT
 * Independently authored analytic bounds for decoded 2D primitives.
 */
#include "musa_scene2d_geometry.h"
#include <float.h>
#include <math.h>

static const double MUSA_PI = 3.14159265358979323846264338327950288;
static int widen(MusaScene2dBox b, MusaScene2dBox *out) {
    b.min_x = nextafter(b.min_x, -INFINITY);
    b.min_y = nextafter(b.min_y, -INFINITY);
    b.max_x = nextafter(b.max_x, INFINITY);
    b.max_y = nextafter(b.max_y, INFINITY);
    if (!isfinite(b.min_x) || !isfinite(b.min_y) ||
        !isfinite(b.max_x) || !isfinite(b.max_y)) return 0;
    *out = b;
    return 1;
}
int musa_scene2d_line_bounds(double x1, double y1, double x2, double y2,
                             MusaScene2dBox *out) {
    if (!out || !isfinite(x1) || !isfinite(y1) ||
        !isfinite(x2) || !isfinite(y2)) return 0;
    return widen((MusaScene2dBox){fmin(x1,x2),fmin(y1,y2),
                                  fmax(x1,x2),fmax(y1,y2)},out);
}
int musa_scene2d_circle_bounds(double cx, double cy, double radius,
                               MusaScene2dBox *out) {
    if (!out || !isfinite(cx) || !isfinite(cy) ||
        !isfinite(radius) || radius <= 0) return 0;
    return widen((MusaScene2dBox){cx-radius,cy-radius,cx+radius,cy+radius},out);
}
static double wrap_angle(double a) {
    const double tau = 2.0*MUSA_PI;
    a = fmod(a,tau);
    return a < 0 ? a+tau : a;
}
static int in_sweep(double angle, double start, double sweep) {
    const double tau = 2.0*MUSA_PI;
    double delta = angle-start;
    if (delta < 0) delta += tau;
    /* Favor false positives over missing a cardinal extremum. */
    const double tolerance = 16.0*DBL_EPSILON*tau;
    return delta <= sweep+tolerance || tau-delta <= tolerance;
}
int musa_scene2d_arc_bounds(double cx, double cy, double radius,
                            double start_rad, double end_rad,
                            MusaScene2dBox *out) {
    if (!out || !isfinite(cx) || !isfinite(cy) ||
        !isfinite(radius) || radius <= 0 ||
        !isfinite(start_rad) || !isfinite(end_rad)) return 0;
    const double tau = 2.0*MUSA_PI;
    /* A full revolution or more has the same extrema as a circle. */
    if (fabs(end_rad-start_rad) >= tau)
        return musa_scene2d_circle_bounds(cx,cy,radius,out);
    const double start = wrap_angle(start_rad);
    const double end = wrap_angle(end_rad);
    const double sweep = end >= start ? end-start : end+tau-start;
    const double sx = cx + radius*cos(start);
    const double sy = cy + radius*sin(start);
    const double ex = cx + radius*cos(end);
    const double ey = cy + radius*sin(end);
    if (!isfinite(sx) || !isfinite(sy) ||
        !isfinite(ex) || !isfinite(ey)) return 0;
    MusaScene2dBox b = {fmin(sx,ex),fmin(sy,ey),fmax(sx,ex),fmax(sy,ey)};
    for (unsigned k=0;k<4;++k) {
        const double a = k*(MUSA_PI/2.0);
        if (!in_sweep(a,start,sweep)) continue;
        const double x = cx + radius*cos(a);
        const double y = cy + radius*sin(a);
        if (!isfinite(x) || !isfinite(y)) return 0;
        b.min_x = fmin(b.min_x,x); b.max_x = fmax(b.max_x,x);
        b.min_y = fmin(b.min_y,y); b.max_y = fmax(b.max_y,y);
    }
    /* Angle reduction and trigonometry may differ by a few ulps across
     * C runtimes. Preserve a conservative outward error margin. */
    const double mx = 32.0*DBL_EPSILON*(fabs(cx)+radius);
    const double my = 32.0*DBL_EPSILON*(fabs(cy)+radius);
    b.min_x -= mx; b.max_x += mx;
    b.min_y -= my; b.max_y += my;
    return widen(b,out);
}

/* A planar ellipse with major vector (ax,ay) and minor vector
 * ratio*(-ay,ax) has x/y extrema at the Euclidean norms of the
 * respective cosine/sine coefficients. No trigonometric sampling or
 * third-party CAD implementation is required. */
int musa_scene2d_ellipse_bounds(double cx, double cy,
                                double ax, double ay, double minor_ratio,
                                MusaScene2dBox *out) {
    if (!out || !isfinite(cx) || !isfinite(cy) ||
        !isfinite(ax) || !isfinite(ay) ||
        !isfinite(minor_ratio) || minor_ratio <= 0.0 ||
        minor_ratio > 1.0 || (ax == 0.0 && ay == 0.0))
        return 0;
    const double minor_x = -ay * minor_ratio;
    const double minor_y = ax * minor_ratio;
    const double rx = hypot(ax, minor_x);
    const double ry = hypot(ay, minor_y);
    if (!isfinite(minor_x) || !isfinite(minor_y) ||
        !isfinite(rx) || !isfinite(ry)) return 0;
    return widen((MusaScene2dBox){cx-rx,cy-ry,cx+rx,cy+ry},out);
}
