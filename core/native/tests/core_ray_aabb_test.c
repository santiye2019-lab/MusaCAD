/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bounds3.h"
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
static void near(double x,double y,double e){
    assert(isfinite(x)&&fabs(x-y)<=e);
}
int main(void){
    const double mn[]={0,0,0},mx[]={2,2,0};
    const double origin[]={.5,.5,4},down[]={0,0,-1};
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,down,4.)==1);
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,down,3.99)==0);
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,down,100.)==1);
    const double offset[]={3,3,4};
    assert(musa_core_bounds3_ray_maybe(mn,mx,offset,down,100.)==0);
    const double side[]={1,0,0};
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,side,100.)==0);
    const double on_surface[]={.5,.5,0};
    assert(musa_core_bounds3_ray_maybe(mn,mx,on_surface,down,0.)==1);
    /* Zero-width slab and WCS with million-unit coordinates. */
    const double wmn[]={1e6,2e6,3e6},wmx[]={1e6+2,2e6+2,3e6};
    const double wo[]={1e6+.5,2e6+.5,3e6+10};
    assert(musa_core_bounds3_ray_maybe(wmn,wmx,wo,down,10.)==1);
    assert(musa_core_bounds3_ray_maybe(wmn,wmx,wo,down,9.9)==0);
    /* Degenerate direction and bad inputs never silently pass. */
    const double zero[]={0,0,0},bad[]={NAN,0,0},badmax[]={-1,2,0};
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,zero,10.)==-1);
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,bad,10.)==-1);
    assert(musa_core_bounds3_ray_maybe(mn,badmax,origin,down,10.)==-1);
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,down,NAN)==-1);
    assert(musa_core_bounds3_ray_maybe(NULL,mx,origin,down,10.)==-1);
    /* Tiny nonzero direction may overflow inverse; do not wrongly cull. */
    const double tiny[]={0,0,-DBL_MIN};
    assert(musa_core_bounds3_ray_maybe(mn,mx,origin,tiny,10.)==1);

    /* Picking's broadphase is integrated in the same portable math path. */
    const float mesh[]={0,0,0,2,0,0,0,2,0,0,0,-2,2,0,-2,0,2,-2};
    const int faces[]={0,1,2,3,4,5};
    MusaCoreMeshRayHit hit={0};
    assert(musa_core_mesh_ray_pick_f32(mesh,6,faces,2,origin,down,10,&hit)==1);
    assert(hit.triangle_index==0);
    near(hit.distance,4.,1e-12);
    const int corrupt[]={0,1,2,3,4,99};
    hit.distance=123.;hit.triangle_index=777;
    assert(musa_core_mesh_ray_pick_f32(mesh,6,corrupt,2,origin,down,10,&hit)==-1);
    assert(hit.distance==123.&&hit.triangle_index==777);
    puts("MusaCAD conservative per-face AABB early ray reject PASS");
    return 0;
}
