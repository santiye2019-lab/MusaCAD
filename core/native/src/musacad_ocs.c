/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_ocs.h"
#include <math.h>
#include <string.h>
static int finite3(const double p[3]) {
    return p && isfinite(p[0]) && isfinite(p[1]) && isfinite(p[2]);
}
static double dot3(const double a[3], const double b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
static void cross3(const double a[3], const double b[3], double out[3]) {
    out[0]=a[1]*b[2]-a[2]*b[1];
    out[1]=a[2]*b[0]-a[0]*b[2];
    out[2]=a[0]*b[1]-a[1]*b[0];
}
static int norm3(const double input[3], double result[3]) {
    if (!finite3(input)) return 0;
    const double m=fmax(fmax(fabs(input[0]),fabs(input[1])),fabs(input[2]));
    if (!(m>0.0) || !isfinite(m)) return 0;
    const double a[3]={input[0]/m,input[1]/m,input[2]/m};
    const double length=hypot(hypot(a[0],a[1]),a[2]);
    if (!(length>0.0) || !isfinite(length)) return 0;
    double next[3];
    for (int i=0;i<3;++i) next[i]=a[i]/length;
    if (!finite3(next)) return 0;
    memcpy(result,next,sizeof(next));
    return 1;
}
int musa_core_ocs_basis(const double extrusion[3], MusaCoreOcsBasis *out) {
    if (!out) return 0;
    MusaCoreOcsBasis next={0};
    if (!norm3(extrusion,next.z)) return 0;
    const double wz[3]={0.0,0.0,1.0};
    const double wy[3]={0.0,1.0,0.0};
    /* DWG arbitrary axis selects a polar reference only when X and Y
     * magnitudes are BOTH below 1/64 in the unit extrusion vector. */
    const double *ref=(fabs(next.z[0])<1.0/64.0 &&
                       fabs(next.z[1])<1.0/64.0) ? wy : wz;
    double xraw[3];
    cross3(ref,next.z,xraw);
    if (!norm3(xraw,next.x)) return 0;
    cross3(next.z,next.x,next.y);
    if (!norm3(next.y,next.y)) return 0;
    *out=next;
    return 1;
}
int musa_core_ocs_to_world(const MusaCoreOcsBasis *basis,
                           const double xy[2], double elevation,
                           double out_xyz[3]) {
    if (!basis || !xy || !out_xyz ||
        !isfinite(xy[0]) || !isfinite(xy[1]) || !isfinite(elevation) ||
        !finite3(basis->x) || !finite3(basis->y) ||
        !finite3(basis->z)) return 0;
    /* Caller-created basis must not introduce shear, scale or reflections. */
    const double *a=basis->x,*b=basis->y,*c=basis->z;
    const double eps=1e-9;
    if (fabs(dot3(a,a)-1.0)>eps || fabs(dot3(b,b)-1.0)>eps ||
        fabs(dot3(c,c)-1.0)>eps || fabs(dot3(a,b))>eps ||
        fabs(dot3(a,c))>eps || fabs(dot3(b,c))>eps) return 0;
    double cross[3];
    cross3(a,b,cross);
    if (dot3(cross,c)<1.0-eps) return 0;
    double next[3];
    for (int i=0;i<3;++i) {
        next[i]=xy[0]*a[i]+xy[1]*b[i]+elevation*c[i];
        if (!isfinite(next[i])) return 0;
    }
    memcpy(out_xyz,next,sizeof(next));
    return 1;
}
