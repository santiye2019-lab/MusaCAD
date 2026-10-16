/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_BREAK2_H
#define MUSACAD_BREAK2_H
/*
 * Cross-platform finite 2D LINE single-point BREAK geometry.
 * WCS inputs; current editor output is float-backed, so guard endpoint
 * collapse before passing to Android/Windows/iOS source edit adapters.
 * A two-point BREAK gap, compound source transaction and DXF/DWG metadata
 * transfer are separate unimplemented acceptance scopes.
 */
#include <math.h>
#include <float.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaCoreBreak2 {
    double first[4],second[4];
} MusaCoreBreak2;

static inline int musa_core_break2_point(const double line[4],
                                          double px,double py,
                                          MusaCoreBreak2 *out) {
    if (!line||!out||!isfinite(px)||!isfinite(py)||
        fabs(px)>1e12||fabs(py)>1e12)return 0;
    for (int i=0;i<4;i++)
        if (!isfinite(line[i])||fabs(line[i])>1e12)return 0;
    const double dx=line[2]-line[0],dy=line[3]-line[1];
    const double length=hypot(dx,dy);
    if(!(length>1e-12)||!isfinite(length))return 0;
    const double t=((px-line[0])*(dx/length)+(py-line[1])*(dy/length))/length;
    if (!isfinite(t)||t<=0.001||t>=0.999)return 0;
    const double sx=line[0]+dx*t,sy=line[1]+dy*t;
    if(!isfinite(sx)||!isfinite(sy)||fabs(sx)>1e12||fabs(sy)>1e12||
       hypot(sx-line[0],sy-line[1])<=1e-12||
       hypot(sx-line[2],sy-line[3])<=1e-12)return 0;
    if (fabs(sx)>FLT_MAX||fabs(sy)>FLT_MAX||
        ((float)sx==(float)line[0]&&(float)sy==(float)line[1])||
        ((float)sx==(float)line[2]&&(float)sy==(float)line[3]))return 0;
    const MusaCoreBreak2 result={{
        line[0],line[1],sx,sy
    },{
        sx,sy,line[2],line[3]
    }};
    *out=result;
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
