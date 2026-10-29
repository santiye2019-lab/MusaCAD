/* SPDX-License-Identifier: MIT
 * Independently authored C11 text placement from elementary affine algebra.
 */
#include "musa_scene2d_text_placement.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *p, size_t pn, const void *q, size_t qn) {
    const uintptr_t a=(uintptr_t)p, b=(uintptr_t)q;
    return a<=b ? b-a<pn : a-b<qn;
}
int musa_scene2d_text_affine(const MusaScene2dTextPlacement *placement,
                             double out[6]) {
    if (!placement || !out ||
        overlaps(placement,sizeof(*placement),out,6u*sizeof(*out))) return 0;
    const double f[]={placement->insertion_x,placement->insertion_y,
        placement->height,placement->width_factor,placement->oblique_rad,
        placement->rotation_rad,placement->anchor_x,placement->anchor_y};
    for (size_t i=0;i<sizeof(f)/sizeof(f[0]);++i)
        if (!isfinite(f[i])) return 0;
    if (placement->height<=0 || placement->width_factor==0) return 0;
    const double c=cos(placement->rotation_rad);
    const double s=sin(placement->rotation_rad);
    const double sx=placement->height*placement->width_factor;
    const double sy=placement->height;
    const double skew=sy*tan(placement->oblique_rad);
    const double a=c*sx, b=s*sx;
    const double cc=c*skew-s*sy, d=s*skew+c*sy;
    const double tx=placement->insertion_x-a*placement->anchor_x-
                    cc*placement->anchor_y;
    const double ty=placement->insertion_y-b*placement->anchor_x-
                    d*placement->anchor_y;
    const double result[]={a,b,cc,d,tx,ty};
    for (size_t i=0;i<6;++i) if (!isfinite(result[i])) return 0;
    for (size_t i=0;i<6;++i) out[i]=result[i];
    return 1;
}
