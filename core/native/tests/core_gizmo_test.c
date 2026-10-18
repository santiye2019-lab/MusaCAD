/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_gizmo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
static void near(double value,double want){
    assert(isfinite(value)&&fabs(value-want)<1e-8);
}
int main(void){
    double xy[2]={42,43};
    const double x[3]={1,0,0},y[3]={0,1,0},z[3]={0,0,1};
    assert(musa_core_gizmo_axis_direction(x,0,0,xy));
    near(xy[0],1);near(xy[1],0);
    double dirs[6]={xy[0],xy[1],0,0,0,-1};
    assert(musa_core_gizmo_axis_direction(y,0,0,xy));
    near(xy[0],0);near(xy[1],0);
    assert(musa_core_gizmo_axis_direction(z,0,0,xy));
    near(xy[0],0);near(xy[1],-1);
    assert(musa_core_gizmo_pick(58,1,dirs,58,16)==0);
    assert(musa_core_gizmo_pick(1,-58,dirs,58,16)==2);
    assert(musa_core_gizmo_pick(100,100,dirs,58,16)==-1);
    assert(musa_core_gizmo_pick(0,0,dirs,58,16)==-1);
    assert(musa_core_gizmo_pick(0,58,dirs,58,16)==-1);
    double angled[3]={0,0,1};
    assert(musa_core_gizmo_axis_direction(angled,0,0,xy));
    near(xy[1],-1);
    assert(musa_core_gizmo_axis_direction(x,1.57079632679489661923,0,xy));
    near(xy[0],0);near(xy[1],0);
    near(musa_core_gizmo_drag_distance(100,0,1,0,10),10);
    near(musa_core_gizmo_drag_distance(0,-25,0,-1,5),5);
    near(musa_core_gizmo_drag_distance(10,15,1,0,5),2);
    assert(isnan(musa_core_gizmo_drag_distance(100,0,0,0,10)));
    assert(isnan(musa_core_gizmo_drag_distance(100,0,0.05,0,10)));
    assert(isnan(musa_core_gizmo_drag_distance(100,0,1,0,0)));
    assert(isnan(musa_core_gizmo_drag_distance(NAN,0,1,0,10)));
    assert(isnan(musa_core_gizmo_drag_distance(DBL_MAX,DBL_MAX,1,0,10)));
    /* Invalid values never partially corrupt the caller's output. */
    double sentinel[2]={42,43};
    assert(!musa_core_gizmo_axis_direction(x,NAN,0,sentinel));
    near(sentinel[0],42);near(sentinel[1],43);
    assert(!musa_core_gizmo_axis_direction(NULL,0,0,sentinel));
    assert(!musa_core_gizmo_axis_direction(x,0,0,NULL));
    assert(musa_core_gizmo_pick(1,1,NULL,20,5)==-1);
    assert(musa_core_gizmo_pick(NAN,1,dirs,20,5)==-1);
    double invalid_dirs[6]={NAN,NAN,0,0,0,-1};
    assert(musa_core_gizmo_pick(0,-20,invalid_dirs,20,5)==2);
    puts("PASS: cross-platform 3D CAD gizmo axes, picking and bounded drag");
    return 0;
}
