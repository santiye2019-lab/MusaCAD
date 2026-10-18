/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_gizmo.h"
#include <math.h>
#include <stddef.h>

#define MIN_VISIBLE_AXIS 0.12
int musa_core_gizmo_axis_direction(const double wcs[3],
                                   double yaw,double pitch,double out_xy[2]){
    if(!wcs||!out_xy||!isfinite(yaw)||!isfinite(pitch)||
       !isfinite(wcs[0])||!isfinite(wcs[1])||!isfinite(wcs[2]))return 0;
    double co=cos(yaw),si=sin(yaw),cp=cos(pitch),sp=sin(pitch);
    double away=si*wcs[0]+co*wcs[1];
    double result[2]={co*wcs[0]-si*wcs[1],sp*away-cp*wcs[2]};
    if(!isfinite(result[0])||!isfinite(result[1]))return 0;
    out_xy[0]=result[0];out_xy[1]=result[1];
    return 1;
}
int musa_core_gizmo_pick(double touch_x,double touch_y,
                         const double axes_xy[6],
                         double handle_pixels,double tolerance_pixels){
    if(!axes_xy||!isfinite(touch_x)||!isfinite(touch_y)||
       !isfinite(handle_pixels)||!isfinite(tolerance_pixels)||
       handle_pixels<=0.0||tolerance_pixels<=0.0)return -1;
    double best=tolerance_pixels*tolerance_pixels;
    if(!isfinite(best))return -1;
    int selected=-1;
    for(int axis=0;axis<3;axis++){
        double vx=axes_xy[2*axis],vy=axes_xy[2*axis+1];
        double len=hypot(vx,vy);
        if(!isfinite(len)||len<MIN_VISIBLE_AXIS)continue;
        double tip_x=vx/len*handle_pixels,tip_y=vy/len*handle_pixels;
        double off_x=touch_x-tip_x,off_y=touch_y-tip_y;
        double d2=off_x*off_x+off_y*off_y;
        if(d2<=best){best=d2;selected=axis;}
    }
    return selected;
}
double musa_core_gizmo_drag_distance(double dx_pixels,double dy_pixels,
                                     double axis_x,double axis_y,
                                     double pixels_per_unit){
    double norm=axis_x*axis_x+axis_y*axis_y;
    if(!isfinite(dx_pixels)||!isfinite(dy_pixels)||!isfinite(norm)||
       norm<MIN_VISIBLE_AXIS*MIN_VISIBLE_AXIS||
       !isfinite(pixels_per_unit)||pixels_per_unit<=0.0)return NAN;
    double distance=(dx_pixels*axis_x+dy_pixels*axis_y)/
                    (pixels_per_unit*norm);
    return isfinite(distance)?distance:NAN;
}
