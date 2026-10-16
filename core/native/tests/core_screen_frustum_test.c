/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_screen_frustum.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    const double eye[16]={1,0,0,0, 0,1,0,0,
                          0,0,1,0, 0,0,0,1};
    MusaCoreScreenFrustum fr={0};
    assert(musa_core_screen_frustum_build(eye,200,100,50,25,150,75,0,&fr));
    const double inner_min[]={-.2,-.2,-.2},inner_max[]={.2,.2,.2};
    const double cross_min[]={.4,-.2,-.2},cross_max[]={.8,.2,.2};
    const double outside_min[]={.7,-.2,-.2},outside_max[]={.9,.2,.2};
    const double behind_min[]={0,0,-3},behind_max[]={.1,.1,-2};
    assert(musa_core_screen_frustum_classify_aabb(&fr,inner_min,inner_max)==2);
    assert(musa_core_screen_frustum_classify_aabb(&fr,cross_min,cross_max)==1);
    assert(musa_core_screen_frustum_classify_aabb(&fr,outside_min,outside_max)==0);
    assert(musa_core_screen_frustum_classify_aabb(&fr,behind_min,behind_max)==0);
    /* Touch y increases down: window from top 0..25 maps y .5..1. */
    assert(musa_core_screen_frustum_build(eye,200,100,50,0,150,25,0,&fr));
    const double top_min[]={0,.7,0},top_max[]={.1,.8,.1};
    const double bottom_min[]={0,-.8,0},bottom_max[]={.1,-.7,.1};
    assert(musa_core_screen_frustum_classify_aabb(&fr,top_min,top_max)==2);
    assert(musa_core_screen_frustum_classify_aabb(&fr,bottom_min,bottom_max)==0);
    /* Same identity matrix under Metal/Direct3D's depth 0..1 range. */
    assert(musa_core_screen_frustum_build(eye,200,100,50,25,150,75,1,&fr));
    const double z_behind_min[]={0,0,-.5},z_behind_max[]={.1,.1,-.1};
    const double z_valid_min[]={0,0,.2},z_valid_max[]={.1,.1,.3};
    assert(musa_core_screen_frustum_classify_aabb(&fr,z_behind_min,z_behind_max)==0);
    assert(musa_core_screen_frustum_classify_aabb(&fr,z_valid_min,z_valid_max)==2);
    /* Perspective camera, 90deg fovy, near 1, far 10.
     * Restrict to central quarter-to-three-quarter screen rectangle.
     */
    const double perspective[16]={1,0,0,0,0,1,0,0,
                                  0,0,-11./9.,-1.,
                                  0,0,-20./9.,0};
    assert(musa_core_screen_frustum_build(perspective,200,200,
                                          50,50,150,150,0,&fr));
    const double p_inside_min[]={-.2,-.2,-3},p_inside_max[]={.2,.2,-2};
    const double p_out_min[]={1.5,-.1,-2},p_out_max[]={1.8,.1,-2};
    const double p_cross_min[]={.9,-.1,-2},p_cross_max[]={1.2,.1,-2};
    assert(musa_core_screen_frustum_classify_aabb(&fr,p_inside_min,p_inside_max)==2);
    assert(musa_core_screen_frustum_classify_aabb(&fr,p_out_min,p_out_max)==0);
    assert(musa_core_screen_frustum_classify_aabb(&fr,p_cross_min,p_cross_max)==1);
    /* High WCS eastings/northings must keep the same spatial result. */
    const double moved[16]={1,0,0,0,0,1,0,0,0,0,1,0,
                             -1000000,2000000,-3000000,1};
    assert(musa_core_screen_frustum_build(moved,200,100,
                                          50,25,150,75,0,&fr));
    const double far_min[]={999999.8,-2000000.2,2999999.8};
    const double far_max[]={1000000.2,-1999999.8,3000000.2};
    assert(musa_core_screen_frustum_classify_aabb(&fr,far_min,far_max)==2);
    /* Failure never publishes output planes or invents class. */
    const double bad[] = {NAN,0,0};
    MusaCoreScreenFrustum guard=fr;
    assert(!musa_core_screen_frustum_build(eye,200,100,50,25,50,75,0,&fr));
    assert(!musa_core_screen_frustum_build(eye,200,100,50,25,150,75,2,&fr));
    assert(!musa_core_screen_frustum_build(NULL,200,100,50,25,150,75,0,&fr));
    assert(!musa_core_screen_frustum_build(eye,200,100,50,25,150,75,0,NULL));
    assert(fr.planes[0][3]==guard.planes[0][3]);
    assert(musa_core_screen_frustum_classify_aabb(&fr,bad,inner_max)==-1);
    assert(musa_core_screen_frustum_classify_aabb(NULL,inner_min,inner_max)==-1);
    puts("MusaCAD portable CAD screen window/crossing frustum AABB PASS");
    return 0;
}
