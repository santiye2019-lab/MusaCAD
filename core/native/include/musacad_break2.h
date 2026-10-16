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
/*
 * BREAK2: two picks create a true gap, preserving drawing orientation.
 * Each result line remains finite and nonzero in float-backed consumers.
 */
static inline int musa_core_break2_gap(const double line[4],
                                       double ax,double ay,double bx,double by,
                                       MusaCoreBreak2 *out){
    if(!line||!out||!isfinite(ax)||!isfinite(ay)||!isfinite(bx)||!isfinite(by)||
       fabs(ax)>1e12||fabs(ay)>1e12||fabs(bx)>1e12||fabs(by)>1e12)return 0;
    for(int i=0;i<4;i++)
        if(!isfinite(line[i])||fabs(line[i])>1e12)return 0;
    const double dx=line[2]-line[0],dy=line[3]-line[1];
    const double length=hypot(dx,dy);
    if(!(length>1e-12)||!isfinite(length))return 0;
    const double t1=((ax-line[0])*(dx/length)+(ay-line[1])*(dy/length))/length;
    const double t2=((bx-line[0])*(dx/length)+(by-line[1])*(dy/length))/length;
    if(!isfinite(t1)||!isfinite(t2))return 0;
    const double lo=fmin(t1,t2),hi=fmax(t1,t2);
    if(lo<=0.001||hi>=0.999||hi-lo<=0.001)return 0;
    const double x1=line[0]+dx*lo,y1=line[1]+dy*lo;
    const double x2=line[0]+dx*hi,y2=line[1]+dy*hi;
    if(!isfinite(x1)||!isfinite(y1)||!isfinite(x2)||!isfinite(y2)||
       fabs(x1)>1e12||fabs(y1)>1e12||fabs(x2)>1e12||fabs(y2)>1e12||
       fabs(x1)>FLT_MAX||fabs(y1)>FLT_MAX||
       fabs(x2)>FLT_MAX||fabs(y2)>FLT_MAX)return 0;
    if(((float)x1==(float)line[0]&&(float)y1==(float)line[1])||
       ((float)x2==(float)line[2]&&(float)y2==(float)line[3])||
       ((float)x1==(float)x2&&(float)y1==(float)y2))return 0;
    const MusaCoreBreak2 result={{line[0],line[1],x1,y1},
                                  {x2,y2,line[2],line[3]}};
    *out=result;
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
