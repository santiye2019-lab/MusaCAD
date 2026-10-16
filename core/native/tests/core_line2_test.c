/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_line2.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near(double a,double b,double eps) {
    assert(isfinite(a) && fabs(a-b)<=eps);
}
int main(void) {
    MusaCoreLine2Hit hit={-7,-7,-7,-7};
    /* A perfectly valid 0.1 mm perpendicular intersection. */
    assert(musa_core_line2_intersect(0,0,0.0001,0,
                                    0.00005,-0.0001,0.00005,0.0001,&hit));
    near(hit.x,0.00005,1e-13);near(hit.y,0,1e-13);
    near(hit.t,0.5,1e-12);near(hit.u,0.5,1e-12);
    /* EXTEND: intersection outside target segment, inside boundary. */
    assert(musa_core_line2_intersect(0,0,0.00004,0,
                                    0.00005,-0.0001,0.00005,0.0001,&hit));
    near(hit.t,1.25,1e-12);near(hit.u,0.5,1e-12);
    MusaCoreLine2Hit untouched={11,22,33,44};
    assert(!musa_core_line2_intersect(0,0,1,0,0,1,1,1,&untouched));
    assert(untouched.x==11&&untouched.y==22&&untouched.t==33&&untouched.u==44);
    assert(!musa_core_line2_intersect(0,0,0,0,0,-1,0,1,&untouched));
    assert(!musa_core_line2_intersect(0,0,1e8,0,0,1,1e8,1.000000001,&untouched));
    assert(!musa_core_line2_intersect(NAN,0,1,0,0,1,0,-1,&untouched));
    assert(!musa_core_line2_intersect(0,0,1,0,2e12,-1,2e12,1,&untouched));
    assert(untouched.x==11&&untouched.y==22&&untouched.t==33&&untouched.u==44);
    /* Translation does not alter local geometry (within FP resolution). */
    assert(musa_core_line2_intersect(1e9,1e9,1e9+0.0001,1e9,
                                    1e9+0.00005,1e9-0.001,
                                    1e9+0.00005,1e9+0.001,&hit));
    near(hit.t,0.5,1e-3); /* quantization near 1e9 is ~1e-7 */
    puts("2D common LINE intersection tests OK");
    return 0;
}
