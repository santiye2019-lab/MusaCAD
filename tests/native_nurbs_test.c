/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exact rational B-spline host regression, independent from DWG/Android. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../app/src/main/cpp/native_nurbs.h"

static void close_to(double found,double expected,double tol){
    if(!isfinite(found)||fabs(found-expected)>tol){
        fprintf(stderr,"NURBS %.16f expected %.16f\n",found,expected);
        exit(1);
    }
}
int main(void){
    double x,y;
    /* Pinned LibreDWG encodes no weight bytes for weighted=false and
     * forcibly leaves serialized control-point w=0 in memory. Regardless
     * of the rational flag, mathematically this means w=1. */
    close_to(musa_nurbs_weight(0.0,0),1.0,0.0);
    close_to(musa_nurbs_weight(42.0,0),1.0,0.0);
    close_to(musa_nurbs_weight(sqrt(.5),1),sqrt(.5),0.0);
    MusaNurbsControl implicit[]={
        {1,0,musa_nurbs_weight(0,0)},
        {1,1,musa_nurbs_weight(0,0)},
        {0,1,musa_nurbs_weight(0,0)}
    };
    double implicit_knots[]={0,0,0,1,1,1};
    assert(musa_nurbs_valid(implicit,3,implicit_knots,6,2));
    assert(musa_nurbs_eval(implicit,3,implicit_knots,2,.5,&x,&y));
    close_to(x,.75,1e-12);
    close_to(y,.75,1e-12);
    /* Degree 3 Bezier, not the broken control-point polygon.
     * P0=(0,0), P1=(0,3), P2=(3,3), P3=(3,0).
     * At u=.5, exact curve=(1.5, 2.25).
     */
    MusaNurbsControl cubic[]={
        {0,0,1},{0,3,1},{3,3,1},{3,0,1}
    };
    double knots3[]={0,0,0,0,1,1,1,1};
    assert(musa_nurbs_valid(cubic,4,knots3,8,3));
    assert(musa_nurbs_preview_points(knots3,4,3)==13);
    assert(musa_nurbs_eval(cubic,4,knots3,3,0.5,&x,&y));
    close_to(x,1.5,1e-12);
    close_to(y,2.25,1e-12);
    assert(musa_nurbs_eval(cubic,4,knots3,3,0.0,&x,&y));
    close_to(x,0,1e-12);close_to(y,0,1e-12);
    assert(musa_nurbs_eval(cubic,4,knots3,3,1.0,&x,&y));
    close_to(x,3,1e-12);close_to(y,0,1e-12);

    /* Exact quarter circle from 3 quadratic rational control points. */
    const double w=sqrt(.5);
    MusaNurbsControl quarter[]={{1,0,1},{1,1,w},{0,1,1}};
    double knots2[]={0,0,0,1,1,1};
    assert(musa_nurbs_valid(quarter,3,knots2,6,2));
    assert(musa_nurbs_eval(quarter,3,knots2,2,.5,&x,&y));
    close_to(x,sqrt(.5),1e-12);
    close_to(y,sqrt(.5),1e-12);
    assert(musa_nurbs_eval(quarter,3,knots2,2,.25,&x,&y));
    close_to(x,0.9297883010624303,1e-12);
    close_to(y,0.3680947095618728,1e-12);

    /* Two quadratic spans with an interior single knot. */
    MusaNurbsControl b[]={
        {0,0,1},{1,2,1},{3,2,1},{4,0,1}
    };
    double k[]={0,0,0,.5,1,1,1};
    assert(musa_nurbs_valid(b,4,k,7,2));
    assert(musa_nurbs_preview_points(k,4,2)==25);
    assert(musa_nurbs_eval(b,4,k,2,0.5,&x,&y));
    close_to(x,2,1e-12);close_to(y,2,1e-12);
    assert(musa_nurbs_eval(b,4,k,2,1,&x,&y));
    close_to(x,4,1e-12);close_to(y,0,1e-12);

    /* An interior knot repeated degree+1 times splits the NURBS into
     * disconnected segments. A single drawn polyline would invent an
     * artificial bridge; refuse instead of displaying false CAD geometry.
     * Repeated interior knots up to degree are still valid. */
    MusaNurbsControl discontinuous_ctrl[]={
        {0,0,1},{1,1,1},{2,0,1},{100,0,1},{101,1,1},{102,0,1}
    };
    double discontinuous_knots[]={0,0,0,.5,.5,.5,1,1,1};
    assert(!musa_nurbs_valid(discontinuous_ctrl,6,discontinuous_knots,9,2));
    double joined_knots[]={0,0,0,.5,.5,.8,1,1,1};
    assert(musa_nurbs_valid(discontinuous_ctrl,6,joined_knots,9,2));

    double backwards[]={0,0,0,1,.5,1,1,1};
    assert(!musa_nurbs_valid(cubic,4,backwards,8,3));
    double nan_knots[]={0,0,0,NAN,1,1};
    assert(!musa_nurbs_valid(quarter,3,nan_knots,6,2));
    MusaNurbsControl bad_w[]={{1,0,1},{1,1,0},{0,1,1}};
    assert(!musa_nurbs_valid(bad_w,3,knots2,6,2));
    assert(!musa_nurbs_valid(quarter,3,knots2,5,2));
    assert(!musa_nurbs_eval(quarter,3,knots2,2,1.5,&x,&y));
    assert(!musa_nurbs_preview_points(NULL,3,2));
    /* 700 positive spans exceed the bounded 8192 points and fail
     * rather than truncating a curve into a misleading partial shape. */
    double *many=(double*)calloc(710,sizeof(double));
    assert(many);
    for(int i=0;i<710;i++)many[i]=i;
    assert(musa_nurbs_preview_points(many,708,1)==0);
    free(many);
    puts("DWG rational SPLINE de Boor: cubic, quarter arc, spans, malformed data PASS");
    return 0;
}
