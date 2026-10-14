/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Regression against natural cubic CAD fit-point interpolation with chord
 * parameters. Expected intermediate coordinates independently calculated
 * from CAD-style global interpolation, not a fit-point control polygon.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../app/src/main/cpp/native_fit_spline.h"

static void need(int ok,const char *what){
    if(!ok){fprintf(stderr,"CAD fit spline: %s\n",what);exit(1);}
}
static void near(double actual,double expected,double tol,const char *what){
    need(isfinite(actual)&&fabs(actual-expected)<=tol,what);
}
int main(void){
    MusaCadFitPoint fit5[]={
        {0,0,0},{1,2,0},{3,1,0},{4,3,0},{6,0,0}
    };
    MusaCadNaturalKnot k5[5]={0};
    need(musa_cad_fit_natural_build(fit5,5,k5),"five fit points solved");
    double x,y;
    for(size_t i=0;i<5;i++){
        need(musa_cad_fit_natural_eval(k5,5,k5[i].t,&x,&y),
            "fit knot evaluated");
        near(x,fit5[i].x,1e-10,"passes exactly through fit point x");
        near(y,fit5[i].y,1e-10,"passes exactly through fit point y");
    }
    need(musa_cad_fit_natural_eval(k5,5,.05,&x,&y),
         "five-point curve at u=.05");
    near(x,.15615278135433253,1e-9,"natural chord fit: x at 0.05");
    near(y,.693871875262185,1e-9,"natural chord fit: y at 0.05");
    need(musa_cad_fit_natural_eval(k5,5,.30,&x,&y),
         "five-point curve at u=.30");
    near(x,1.7562893356005087,1e-9,"natural chord fit x at 0.30");
    near(y,1.610691221825416,1e-9,"natural chord fit y at 0.30");
    need(musa_cad_fit_natural_eval(k5,5,.72,&x,&y),
         "five-point curve at u=.72");
    near(x,4.311732577789597,1e-9,"natural chord fit x at 0.72");
    near(y,3.1346570906030355,1e-9,"natural chord fit y at 0.72");
    /* Curve is not the broken line between adjacent fit points: at u=.05
     * (inside the first chord) it must depart from the straight segment. */
    need(musa_cad_fit_natural_eval(k5,5,.05,&x,&y),
         "cubic quarter segment re-evaluation");
    double straight_y=2.0*(.05/k5[1].t);
    need(fabs(y-straight_y)>0.1,"curve must not degrade into fit-point polygon");

    /* Independent CAD-style global interpolation reference with explicit
     * endpoint tangent directions. The derivatives have chord-length
     * magnitude and differ materially from the natural endpoint curve. */
    MusaCadNaturalKnot clamped5[5]={0};
    need(musa_cad_fit_clamped_build(fit5,5,
         (MusaCadFitPoint){1,0,0},(MusaCadFitPoint){-1,0,0},
         clamped5),"both explicit end tangents solved");
    need(musa_cad_fit_natural_eval(clamped5,5,.05,&x,&y),
         "clamped spline at .05");
    near(x,.3866749624061828,1e-9,"tangent clamped x at .05");
    near(y,.24372559551065884,1e-9,"tangent clamped y at .05");
    need(musa_cad_fit_natural_eval(clamped5,5,.30,&x,&y),
         "clamped spline at .30");
    near(x,1.7101663427222205,1e-9,"tangent clamped x at .30");
    near(y,1.7263552158487832,1e-9,"tangent clamped y at .30");
    need(musa_cad_fit_natural_eval(clamped5,5,.72,&x,&y),
         "clamped spline at .72");
    near(x,4.6491903440757785,1e-9,"tangent clamped x at .72");
    near(y,2.8249423436076784,1e-9,"tangent clamped y at .72");
    need(!musa_cad_fit_clamped_build(fit5,5,
         (MusaCadFitPoint){0,0,0},(MusaCadFitPoint){-1,0,0},
         clamped5),"zero-length explicit tangent must fail");
    need(!musa_cad_fit_clamped_build(fit5,5,
         (MusaCadFitPoint){NAN,0,0},(MusaCadFitPoint){-1,0,0},
         clamped5),"NaN tangent must fail");

    /* One explicit endpoint tangent must not be rejected or guessed.
     * Reference values computed independently from a mixed natural/clamped
     * cubic tridiagonal interpolation system with chord-length parameters.
     */
    MusaCadNaturalKnot mixed_start[5]={0},mixed_end[5]={0};
    need(musa_cad_fit_mixed_build(fit5,5,
         (MusaCadFitPoint){1,0,0},(MusaCadFitPoint){0,0,0},
         mixed_start),"start-only explicit tangent solved");
    need(mixed_start[4].mx==0.&&mixed_start[4].my==0.,
         "unspecified end is natural");
    need(musa_cad_fit_natural_eval(mixed_start,5,.05,&x,&y),
         "start clamped curve at .05");
    near(x,.3887702282714224,1e-9,"mixed start x at .05");
    near(y,.2417459171111591,1e-9,"mixed start y at .05");
    need(musa_cad_fit_natural_eval(mixed_start,5,.72,&x,&y),
         "start clamped curve at .72");
    near(x,4.302594902400962,1e-9,"mixed start x at .72");
    near(y,3.1524174963469447,1e-9,"mixed start y at .72");

    need(musa_cad_fit_mixed_build(fit5,5,
         (MusaCadFitPoint){0,0,0},(MusaCadFitPoint){-1,0,0},
         mixed_end),"end-only explicit tangent solved");
    need(mixed_end[0].mx==0.&&mixed_end[0].my==0.,
         "unspecified start is natural");
    need(musa_cad_fit_natural_eval(mixed_end,5,.05,&x,&y),
         "end clamped curve at .05");
    near(x,.14973151058483003,1e-9,"mixed end x at .05");
    near(y,.6999043524686044,1e-9,"mixed end y at .05");
    need(musa_cad_fit_natural_eval(mixed_end,5,.72,&x,&y),
         "end clamped curve at .72");
    near(x,4.6566062685003695,1e-9,"mixed end x at .72");
    near(y,2.8106647294356084,1e-9,"mixed end y at .72");
    for(size_t i=0;i<5;i++){
        need(musa_cad_fit_natural_eval(mixed_start,5,mixed_start[i].t,
              &x,&y),"start-mixed fit knot");
        near(x,fit5[i].x,1e-10,"start-mixed passes fit x");
        near(y,fit5[i].y,1e-10,"start-mixed passes fit y");
        need(musa_cad_fit_natural_eval(mixed_end,5,mixed_end[i].t,
              &x,&y),"end-mixed fit knot");
        near(x,fit5[i].x,1e-10,"end-mixed passes fit x");
        near(y,fit5[i].y,1e-10,"end-mixed passes fit y");
    }
    need(!musa_cad_fit_mixed_build(fit5,5,
         (MusaCadFitPoint){0,0,0},(MusaCadFitPoint){0,0,0},
         mixed_start),"both natural endpoints need natural builder");
    need(!musa_cad_fit_mixed_build(fit5,5,
         (MusaCadFitPoint){1,0,0},(MusaCadFitPoint){-1,0,0},
         mixed_start),"both explicit endpoints need clamped builder");
    need(!musa_cad_fit_mixed_build(fit5,5,
         (MusaCadFitPoint){NAN,0,0},(MusaCadFitPoint){0,0,0},
         mixed_start),"nonfinite mixed tangent rejected");

    MusaCadFitPoint fit6[]={
        {0,0,0},{1,0,0},{2,1,0},{3,1.5,0},{5,2,0},{8,0,0}
    };
    MusaCadNaturalKnot k6[6]={0};
    need(musa_cad_fit_natural_build(fit6,6,k6),
         "six-point R2013-sized fit curve");
    for(size_t i=0;i<6;i++){
        need(musa_cad_fit_natural_eval(k6,6,k6[i].t,&x,&y),
             "six-point fit evaluated");
        near(x,fit6[i].x,1e-10,"six-point x interpolation");
        near(y,fit6[i].y,1e-10,"six-point y interpolation");
    }
    need(k6[0].mx==0&&k6[0].my==0&&
         k6[5].mx==0&&k6[5].my==0,
         "natural cubic endpoints have zero second derivative");
    need(!musa_cad_fit_natural_eval(k6,6,1.1,&x,&y),
         "out-of-domain interpolation refused");
    MusaCadFitPoint duplicate[]={
        {0,0,0},{1,1,0},{1,1,0},{2,0,0}
    };
    MusaCadNaturalKnot dupout[4]={0};
    need(!musa_cad_fit_natural_build(duplicate,4,dupout),
         "duplicate fit points rejected rather than dividing by zero");
    MusaCadFitPoint invalid[]={
        {0,0,0},{1,NAN,0},{2,0,0}
    };
    MusaCadNaturalKnot badout[3]={0};
    need(!musa_cad_fit_natural_build(invalid,3,badout),
         "nonfinite fit points refused");
    need(!musa_cad_fit_natural_build(fit5,2,k5),
         "invalid spline point count refused");
    need(musa_cad_fit_zero_tangent(0,0,0),
         "unspecified endpoint tangent detected");
    need(!musa_cad_fit_zero_tangent(1,0,0),
         "explicit tangent never silently ignored");
    puts("Natural cubic DWG fit SPLINE: five/six points, CAD reference, malformed and tangent safety PASS");
    return 0;
}
