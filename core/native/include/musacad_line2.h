/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_LINE2_H
#define MUSACAD_LINE2_H
/*
 * Portable WCS 2D LINE support for Android, Windows and iOS. Evaluate the
 * infinite supporting lines; caller decides TRIM/EXTEND based on t/u ranges.
 * This does not trim arcs, polyline bulges, XREFs, or source-DWG entities.
 *
 * Relative angular test avoids wrongly rejecting two short perpendicular
 * lines merely because their unnormalized cross product is < 1e-7.
 * No output is changed for invalid, degenerate, parallel or out-of-budget
 * geometry. Drawing coordinates are bounded to the editor's +/-1e12 budget.
 */
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaCoreLine2Hit {
    double x,y,t,u;
} MusaCoreLine2Hit;

static inline int musa_core_line2_intersect(
    double ax,double ay,double bx,double by,
    double cx,double cy,double dx,double dy,
    MusaCoreLine2Hit *out) {
    if (!out) return 0;
    const double v[8]={ax,ay,bx,by,cx,cy,dx,dy};
    for (int i=0;i<8;i++)
        if (!isfinite(v[i]) || fabs(v[i])>1e12) return 0;
    const double rx=bx-ax,ry=by-ay,sx=dx-cx,sy=dy-cy;
    const double ra=hypot(rx,ry),rb=hypot(sx,sy);
    if (!(ra>1e-12) || !(rb>1e-12) || !isfinite(ra) || !isfinite(rb)) return 0;
    const double ux=rx/ra,uy=ry/ra,vx=sx/rb,vy=sy/rb;
    const double denom=ux*vy-uy*vx;
    if (!isfinite(denom) || fabs(denom)<=1e-12) return 0;
    const double qx=cx-ax,qy=cy-ay;
    const double da=(qx*vy-qy*vx)/denom;
    const double db=(qx*uy-qy*ux)/denom;
    const MusaCoreLine2Hit hit={ax+da*ux,ay+da*uy,da/ra,db/rb};
    if (!isfinite(hit.x)||!isfinite(hit.y)||!isfinite(hit.t)||
        !isfinite(hit.u)||fabs(hit.x)>1e12||fabs(hit.y)>1e12)
        return 0;
    *out=hit;
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
