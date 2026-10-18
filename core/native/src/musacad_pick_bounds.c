/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_pick_bounds.h"
#include <math.h>
/* Match the existing Java conservative float intermediates for DXF edits:
 * only rejection of candidates changes behavior; false positives are safe. */
int musa_core_pick_bounds_f32(const float *xy,size_t n,int kind,
                              float x,float y,float tolerance){
    if(!xy||n<2u||!isfinite(x)||!isfinite(y)||
       !isfinite(tolerance)||tolerance<0.f)return 1;
    if(kind<0||kind>3||kind==3)return 1;
    float min_x=INFINITY,min_y=INFINITY,max_x=-INFINITY,max_y=-INFINITY;
    if(kind==1||kind==2){
        if(n<(kind==1?4u:8u))return 1;
        const size_t center=kind==1?0u:6u;
        float dx=xy[kind==1?2u:0u]-xy[center];
        float dy=xy[kind==1?3u:1u]-xy[center+1u];
        float radius=(float)hypot((double)dx,(double)dy);
        if(!isfinite(radius))return 1;
        min_x=xy[center]-radius;max_x=xy[center]+radius;
        min_y=xy[center+1u]-radius;max_y=xy[center+1u]+radius;
    }else{
        for(size_t i=0;i+1u<n;i+=2u){
            if(!isfinite(xy[i])||!isfinite(xy[i+1u]))return 1;
            if(xy[i]<min_x)min_x=xy[i];
            if(xy[i]>max_x)max_x=xy[i];
            if(xy[i+1u]<min_y)min_y=xy[i+1u];
            if(xy[i+1u]>max_y)max_y=xy[i+1u];
        }
    }
    if(!isfinite(min_x)||!isfinite(min_y)||
       !isfinite(max_x)||!isfinite(max_y))return 1;
    return (double)x+(double)tolerance>=(double)min_x&&
           (double)x-(double)tolerance<=(double)max_x&&
           (double)y+(double)tolerance>=(double)min_y&&
           (double)y-(double)tolerance<=(double)max_y;
}
