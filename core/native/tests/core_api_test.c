/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static int near(double a,double b,double e) {
    return isfinite(a) && fabs(a-b)<=e;
}
int main(void) {
    assert(musa_core_abi_version()==MUSA_CORE_ABI_VERSION);
    assert(MUSA_CORE_ABI_VERSION==1u);
    assert(musa_core_effective_weight(0.0,0)==1.0);
    assert(musa_core_effective_weight(99.0,0)==1.0);
    const double w=sqrt(0.5);
    assert(near(musa_core_effective_weight(w,1),w,0.0));
    const MusaCoreNurbsControl quarter[]={{1,0,1},{1,1,w},{0,1,1}};
    const double knots[]={0,0,0,1,1,1};
    double x=0,y=0;
    assert(musa_core_nurbs_validate(quarter,3,knots,6,2));
    assert(musa_core_nurbs_preview_points(knots,3,2)==13);
    assert(musa_core_nurbs_evaluate(quarter,3,knots,2,.5,&x,&y));
    assert(near(x,sqrt(0.5),1e-12) && near(y,sqrt(0.5),1e-12));
    assert(musa_core_nurbs_evaluate(quarter,3,knots,2,0,&x,&y));
    assert(near(x,1,1e-12) && near(y,0,1e-12));
    assert(musa_core_nurbs_evaluate(quarter,3,knots,2,1,&x,&y));
    assert(near(x,0,1e-12) && near(y,1,1e-12));
    const MusaCoreNurbsControl cubic[]={{0,0,1},{0,3,1},{3,3,1},{3,0,1}};
    const double cubic_knots[]={0,0,0,0,1,1,1,1};
    assert(musa_core_nurbs_validate(cubic,4,cubic_knots,8,3));
    assert(musa_core_nurbs_evaluate(cubic,4,cubic_knots,3,.5,&x,&y));
    assert(near(x,1.5,1e-12) && near(y,2.25,1e-12));
    const double out_of_order[]={0,0,0,1,.5,1};
    assert(!musa_core_nurbs_validate(quarter,3,out_of_order,6,2));
    MusaCoreNurbsControl bad[3]={{1,0,1},{1,1,0},{0,1,1}};
    assert(!musa_core_nurbs_validate(bad,3,knots,6,2));
    /* Public API refuses malformed geometry even without a separate validate. */
    assert(!musa_core_nurbs_evaluate(bad,3,knots,2,.5,&x,&y));
    assert(!musa_core_nurbs_evaluate(quarter,3,knots,2,-1,&x,&y));
    assert(!musa_core_nurbs_evaluate(quarter,3,knots,2,.5,NULL,&y));
    assert(!musa_core_nurbs_evaluate(quarter,3,knots,2,.5,&x,NULL));
    assert(!musa_core_nurbs_evaluate(quarter,3,knots,2,NAN,&x,&y));
    assert(!musa_core_nurbs_evaluate(NULL,3,knots,2,.5,&x,&y));
    puts("MusaCAD C ABI 1: rational NURBS, bounds, legacy DWG weights PASS");
    return 0;
}
