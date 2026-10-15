/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
static void close_to(double x,double y,double e){assert(isfinite(x)&&fabs(x-y)<=e);}
int main(void){
    const float triangle[]={0,0,0,3,0,0,0,4,0};
    const int t[]={0,1,2};
    MusaCoreMeshSurface r={99,98,97,96,88,87};
    assert(musa_core_mesh_surface_f32(triangle,3,t,1,&r));
    close_to(r.area,6.,1e-12);
    close_to(r.centroid_x,1.,1e-12);
    close_to(r.centroid_y,4./3.,1e-12);
    close_to(r.centroid_z,0.,1e-12);
    assert(r.triangle_count==1&&r.degenerate_count==0);
    const float cube[]={0,0,0,1,0,0,1,1,0,0,1,0,
                        0,0,1,1,0,1,1,1,1,0,1,1};
    const int faces[]={0,1,2,0,2,3,4,6,5,4,7,6,
                       0,4,5,0,5,1,1,5,6,1,6,2,
                       2,6,7,2,7,3,3,7,4,3,4,0};
    assert(musa_core_mesh_surface_f32(cube,8,faces,12,&r));
    close_to(r.area,6.,1e-12);
    close_to(r.centroid_x,.5,1e-12);
    close_to(r.centroid_y,.5,1e-12);
    close_to(r.centroid_z,.5,1e-12);
    const float world[]={1000000.f,1000000.f,1000000.f,
                         1000003.f,1000000.f,1000000.f,
                         1000000.f,1000004.f,1000000.f};
    assert(musa_core_mesh_surface_f32(world,3,t,1,&r));
    close_to(r.area,6.,1e-12);
    close_to(r.centroid_x,1000001.,1e-9);
    close_to(r.centroid_y,1000000.+4./3.,1e-9);
    const int deg[]={0,0,2};
    assert(musa_core_mesh_surface_f32(triangle,3,deg,1,&r));
    assert(r.area==0.&&r.degenerate_count==1&&isnan(r.centroid_x));
    assert(musa_core_mesh_surface_f32(NULL,0,NULL,0,&r));
    assert(r.area==0.&&r.triangle_count==0&&isnan(r.centroid_z));
    /* Matching Java behavior: float deltas must first widen to double. */
    const float max=FLT_MAX;
    const float extreme[]={max,max,0,-max,max,0,max,-max,0};
    assert(musa_core_mesh_surface_f32(extreme,3,t,1,&r));
    assert(isfinite(r.area)&&r.area>1e76);
    const int bad_index[]={0,1,3};
    const float invalid[]={0,0,0,NAN,0,0,0,1,0};
    const int incomplete[]={0,1};
    (void)incomplete;
    r.area=123.;r.centroid_x=789.;r.triangle_count=42;
    assert(!musa_core_mesh_surface_f32(triangle,3,bad_index,1,&r));
    assert(!musa_core_mesh_surface_f32(invalid,3,t,1,&r));
    assert(!musa_core_mesh_surface_f32(triangle,3,t,4000001u,&r));
    assert(!musa_core_mesh_surface_f32(triangle,3,t,1,NULL));
    assert(!musa_core_mesh_surface_f32(NULL,3,t,1,&r));
    assert(r.area==123.&&r.centroid_x==789.&&r.triangle_count==42);
    puts("MusaCAD portable 3D mesh surface area and XYZ centroid PASS");
    return 0;
}
