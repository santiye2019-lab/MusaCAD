/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bulge.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
static void near(double got,double expected,double tol){
    assert(isfinite(got));assert(fabs(got-expected)<=tol);
}
int main(void){
    MusaCoreBulgeArc arc={11,12,13,14,15};
    const double pi=acos(-1.0);
    /* Same source math drives LWPOLYLINE and HATCH first paint. */
    assert(musa_core_bulge_to_arc(1,0,0,1,tan(pi/8.0),&arc));
    near(arc.cx,0,1e-12);near(arc.cy,0,1e-12);
    near(arc.radius,1,1e-12);near(arc.start,0,1e-12);
    near(arc.sweep,pi/2.0,1e-12);
    assert(musa_core_bulge_to_arc(0,1,1,0,-tan(pi/8.0),&arc));
    near(arc.cx,0,1e-12);near(arc.cy,0,1e-12);
    near(arc.start,pi/2.0,1e-12);near(arc.sweep,-pi/2.0,1e-12);

    assert(musa_core_bulge_to_arc(0,0,2,0,1,&arc));
    near(arc.cx,1,1e-12);near(arc.cy,0,1e-12);
    near(arc.radius,1,1e-12);near(arc.sweep,pi,1e-12);
    assert(musa_core_bulge_to_arc(0,0,2,0,-1,&arc));
    near(arc.radius,1,1e-12);near(arc.sweep,-pi,1e-12);

    /* Large civil coordinates do not require recentering before conversion. */
    assert(musa_core_bulge_to_arc(1e8+1,1e8,1e8,1e8+1,tan(pi/8.0),&arc));
    near(arc.cx,1e8,1e-7);near(arc.cy,1e8,1e-7);
    near(arc.radius,1,1e-12);

    /* Straight, corrupt and overflow values must not corrupt output. */
    MusaCoreBulgeArc original={3,4,5,6,7};
    arc=original;
    assert(!musa_core_bulge_to_arc(0,0,2,0,0,&arc));
    assert(!musa_core_bulge_to_arc(0,0,2,0,1e-13,&arc));
    assert(!musa_core_bulge_to_arc(0,0,0,0,1,&arc));
    assert(!musa_core_bulge_to_arc(0,0,2,0,NAN,&arc));
    assert(!musa_core_bulge_to_arc(INFINITY,0,2,0,1,&arc));
    assert(!musa_core_bulge_to_arc(DBL_MAX,0,-DBL_MAX,0,1,&arc));
    assert(!musa_core_bulge_to_arc(0,0,2,0,1e160,&arc));
    assert(!musa_core_bulge_to_arc(0,0,2,0,1,NULL));
    near(arc.cx,original.cx,0);near(arc.sweep,original.sweep,0);
    puts("Shared MusaCAD DXF bulge arcs: LWPOLYLINE/HATCH, WCS, CW/CCW, invalid PASS");
    return 0;
}
