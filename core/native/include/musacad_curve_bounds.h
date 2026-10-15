/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_CURVE_BOUNDS_H
#define MUSACAD_CORE_CURVE_BOUNDS_H
/*
 * CAD scene bounds for x=Cx+Ux*cos(t)+Vx*sin(t) and
 * y=Cy+Uy*cos(t)+Vy*sin(t), t in [start,start+sweep].
 * Positive and negative arc directions, elliptical axes and translated
 * WCS coordinates all use the same implementation on mobile and desktop.
 * Result is unchanged if geometry is invalid. No Android/Win32/Apple SDK.
 */
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaCoreCurveBounds {
    double min_x,min_y,max_x,max_y;
} MusaCoreCurveBounds;

static inline int musa_core_curve_bounds(
        double cx,double cy,double ux,double uy,double vx,double vy,
        double start,double sweep,MusaCoreCurveBounds *out) {
    const double pi=3.14159265358979323846264338327950288;
    const double tau=2.0*pi;
    if(!out||!isfinite(cx)||!isfinite(cy)||!isfinite(ux)||
       !isfinite(uy)||!isfinite(vx)||!isfinite(vy)||
       !isfinite(start)||!isfinite(sweep)||fabs(sweep)<1e-12||
       fabs(sweep)>tau+1e-9)return 0;
    const double rx=hypot(ux,vx),ry=hypot(uy,vy);
    if(!isfinite(rx)||!isfinite(ry))return 0;
    MusaCoreCurveBounds b;
    if(fabs(sweep)>=tau-1e-12) {
        b.min_x=cx-rx;b.max_x=cx+rx;b.min_y=cy-ry;b.max_y=cy+ry;
    } else {
        double t=start;
        double x=cx+ux*cos(t)+vx*sin(t),y=cy+uy*cos(t)+vy*sin(t);
        if(!isfinite(x)||!isfinite(y))return 0;
        b.min_x=b.max_x=x;b.min_y=b.max_y=y;
        t=start+sweep;
        x=cx+ux*cos(t)+vx*sin(t);y=cy+uy*cos(t)+vy*sin(t);
        if(!isfinite(x)||!isfinite(y))return 0;
        if(x<b.min_x)b.min_x=x;if(x>b.max_x)b.max_x=x;
        if(y<b.min_y)b.min_y=y;if(y>b.max_y)b.max_y=y;
        /* Stationary angles for the two WCS coordinate functions.
         * A partial arc must not get full-ellipse extents.
         */
        const double extrema[4]={
            atan2(vx,ux),atan2(vx,ux)+pi,
            atan2(vy,uy),atan2(vy,uy)+pi
        };
        for(int k=0;k<4;k++){
            const double angle=extrema[k];
            double dist=fmod(sweep>0?angle-start:start-angle,tau);
            if(!isfinite(dist))return 0;
            if(dist<0)dist+=tau;
            if(dist>fabs(sweep)+1e-12)continue;
            x=cx+ux*cos(angle)+vx*sin(angle);
            y=cy+uy*cos(angle)+vy*sin(angle);
            if(!isfinite(x)||!isfinite(y))return 0;
            if(x<b.min_x)b.min_x=x;if(x>b.max_x)b.max_x=x;
            if(y<b.min_y)b.min_y=y;if(y>b.max_y)b.max_y=y;
        }
    }
    if(!isfinite(b.min_x)||!isfinite(b.min_y)||
       !isfinite(b.max_x)||!isfinite(b.max_y)||
       b.min_x>b.max_x||b.min_y>b.max_y)return 0;
    *out=b;return 1;
}
#ifdef __cplusplus
}
#endif
#endif
