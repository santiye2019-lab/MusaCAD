/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_BULGE_H
#define MUSACAD_CORE_BULGE_H
/*
 * The same DXF bulge-to-circular-arc geometry for Android, Windows and iOS.
 * A signed bulge describes sweep=4*atan(bulge); coordinates are drawing OCS
 * until the host's existing OCS/WCS transform is applied.
 *
 * Return 1 for a valid arc and 0 for a straight/degenerate/unsupported span.
 * On failure *out remains unmodified, so callers can keep the source line
 * rather than writing fabricated arc coordinates into a DWG preview.
 */
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaCoreBulgeArc {
    double cx, cy, radius, start, sweep;
} MusaCoreBulgeArc;

static inline int musa_core_bulge_to_arc(
    double ax,double ay,double bx,double by,double bulge,
    MusaCoreBulgeArc *out) {
    if(!out||!isfinite(ax)||!isfinite(ay)||!isfinite(bx)||
       !isfinite(by)||!isfinite(bulge)||fabs(bulge)<1e-12)return 0;
    double dx=bx-ax,dy=by-ay,chord=hypot(dx,dy);
    if(!isfinite(chord)||chord<1e-12)return 0;
    /* Preserve the existing native first-paint formula evaluation order. */
    double theta=4.0*atan(bulge);
    double h=chord*(1.0-bulge*bulge)/(4.0*bulge);
    double nx=-dy/chord,ny=dx/chord;
    double cx=(ax+bx)*0.5+nx*h,cy=(ay+by)*0.5+ny*h;
    double radius=chord*(1.0+bulge*bulge)/(4.0*fabs(bulge));
    double start=atan2(ay-cy,ax-cx);
    if(!isfinite(cx)||!isfinite(cy)||!isfinite(radius)||
       !isfinite(start)||!isfinite(theta)||radius<=0||
       !isfinite(cx+radius)||!isfinite(cy+radius))return 0;
    MusaCoreBulgeArc result={cx,cy,radius,start,theta};
    *out=result;
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
