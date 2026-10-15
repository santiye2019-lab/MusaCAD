/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_BOUNDS3_H
#define MUSACAD_CORE_BOUNDS3_H
/*
 * Conservative CAD WCS axis-aligned bounding box / ray broad phase.
 * Shared C11, including MSVC/Android NDK/iPhoneOS, without platform headers.
 * Ray direction must be finite and nonzero; t is the ray parameter, in
 * drawing distance units only if the direction has been normalized.
 *
 *  1: AABB may intersect within [0,max_t] (false positives allowed).
 *  0: Definitely does not intersect.
 * -1: Invalid inputs.
 *
 * Use this ONLY for rejection before an exact triangle intersection;
 * tolerance deliberately favors retaining borderline candidates. This is
 * per-face broad phase, NOT a BVH or asymptotically sublinear 3D picking.
 */
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline int musa_core_bounds3_ray_maybe(
        const double minimum[3],const double maximum[3],
        const double origin[3],const double direction[3],double max_t) {
    if(!minimum||!maximum||!origin||!direction||
       !isfinite(max_t)||max_t<0.)return -1;
    int has_direction=0;
    for(int i=0;i<3;i++){
        if(!isfinite(minimum[i])||!isfinite(maximum[i])||
           !isfinite(origin[i])||!isfinite(direction[i])||
           minimum[i]>maximum[i])return -1;
        if(direction[i]!=0.)has_direction=1;
    }
    if(!has_direction)return -1;
    double tmin=0.,tmax=max_t;
    for(int i=0;i<3;i++){
        /*
         * Expanded slabs prevent float-to-double and WCS subtraction
         * rounding from excluding a truly intersecting triangle.
         * If the subtraction/division overflows, do not cull on that axis.
         */
        const double scale=fmax(fabs(origin[i]),
                           fmax(fabs(minimum[i]),fabs(maximum[i])));
        const double epsilon=fmax(1e-10,4e-14*scale);
        const double lo=minimum[i]-epsilon;
        const double hi=maximum[i]+epsilon;
        if(!isfinite(lo)||!isfinite(hi))continue;
        if(direction[i]==0.){
            if(origin[i]<lo||origin[i]>hi)return 0;
            continue;
        }
        double enter=(lo-origin[i])/direction[i];
        double leave=(hi-origin[i])/direction[i];
        if(!isfinite(enter)||!isfinite(leave))continue;
        if(enter>leave){double temp=enter;enter=leave;leave=temp;}
        if(enter>tmin)tmin=enter;
        if(leave<tmax)tmax=leave;
        if(tmin>tmax)return 0;
    }
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
