/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_NATIVE_NURBS_H
#define MUSACAD_NATIVE_NURBS_H
/* Read-only rational B-spline curve evaluation for native DWG first paint.
 * Evaluates real knot/weight/control-point geometry rather than drawing the
 * control polygon (which is usually NOT the spline). Bounded for Android.
 */
#include <math.h>
#include <stddef.h>
#define MUSA_NURBS_MAX_DEGREE 16
#define MUSA_NURBS_MAX_CONTROL 4096
#define MUSA_NURBS_MAX_PREVIEW_POINTS 8192
#define MUSA_NURBS_SAMPLES_PER_KNOT_SPAN 12

typedef struct {double x,y,w;} MusaNurbsControl;

/* LibreDWG's pinned dwg.spec skips the w payload and explicitly sets w=0
 * for unweighted SPLINE control points. That 0 is a serialization sentinel,
 * NOT a rational zero weight. Implicit mathematical weight is 1. */
static double musa_nurbs_weight(double stored,int weighted){
    return weighted?stored:1.0;
}

static int musa_nurbs_valid(const MusaNurbsControl *ctrl,size_t n,
                            const double *knots,size_t nk,int degree) {
    if(!ctrl||!knots||n<2||n>MUSA_NURBS_MAX_CONTROL||
       degree<1||degree>MUSA_NURBS_MAX_DEGREE||
       (size_t)degree>=n||nk!=n+(size_t)degree+1u)
        return 0;
    for(size_t i=0;i<n;i++){
        if(!isfinite(ctrl[i].x)||!isfinite(ctrl[i].y)||
           !isfinite(ctrl[i].w)||ctrl[i].w<=0.)return 0;
    }
    for(size_t i=0;i<nk;i++)
        if(!isfinite(knots[i])||(i&&knots[i]<knots[i-1]))return 0;
    return knots[degree]<knots[n];
}

/* Call after musa_nurbs_valid(); only allocated stack buffers (17 points).
 * Span i with knot[i] <= u < knot[i+1]; at upper endpoint use last span.
 * Homogeneous (w*x, w*y, w) de Boor supports rational conics exactly.
 */
static int musa_nurbs_eval(const MusaNurbsControl *ctrl,size_t n,
                           const double *knots,int degree,double u,
                           double *ox,double *oy) {
    if(!ctrl||!knots||!ox||!oy||degree<1||
       degree>MUSA_NURBS_MAX_DEGREE||n<= (size_t)degree||
       !isfinite(u)||u<knots[degree]||u>knots[n])
        return 0;
    size_t lo=(size_t)degree,hi=n;
    if(u==knots[n])lo=n-1;
    else {
        while(lo+1<hi){
            size_t mid=lo+(hi-lo)/2;
            if(knots[mid]<=u)lo=mid;
            else hi=mid;
        }
        if(lo>=n||knots[lo]>u||knots[lo+1]<=u)return 0;
    }
    double d[MUSA_NURBS_MAX_DEGREE+1][3];
    for(int j=0;j<=degree;j++){
        size_t index=lo-(size_t)degree+(size_t)j;
        double w=ctrl[index].w;
        d[j][0]=ctrl[index].x*w;
        d[j][1]=ctrl[index].y*w;
        d[j][2]=w;
        if(!isfinite(d[j][0])||!isfinite(d[j][1]))return 0;
    }
    for(int level=1;level<=degree;level++){
        for(int j=degree;j>=level;j--){
            size_t a=lo-(size_t)degree+(size_t)j;
            size_t b=lo+(size_t)j-(size_t)level+1u;
            double span=knots[b]-knots[a];
            double alpha=span>0.0?(u-knots[a])/span:0.0;
            if(!isfinite(alpha)||alpha< -1e-10||alpha>1.+1e-10)return 0;
            if(alpha<0.)alpha=0.;
            if(alpha>1.)alpha=1.;
            for(int k=0;k<3;k++)
                d[j][k]=(1.-alpha)*d[j-1][k]+alpha*d[j][k];
        }
    }
    double w=d[degree][2];
    if(!isfinite(w)||w<=0.)return 0;
    *ox=d[degree][0]/w;
    *oy=d[degree][1]/w;
    return isfinite(*ox)&&isfinite(*oy);
}

/* Count unique nonzero knot spans. Never return a silent partial curve. */
static int musa_nurbs_preview_points(const double *knots,size_t n,int degree){
    if(!knots||n<2||degree<1||(size_t)degree>=n)return 0;
    size_t spans=0;
    for(size_t i=(size_t)degree;i<n;i++){
        if(!isfinite(knots[i])||!isfinite(knots[i+1])||
           knots[i+1]<knots[i])return 0;
        if(knots[i+1]>knots[i])spans++;
    }
    if(spans==0||spans>(MUSA_NURBS_MAX_PREVIEW_POINTS-1)/
            MUSA_NURBS_SAMPLES_PER_KNOT_SPAN)return 0;
    return 1+(int)(spans*MUSA_NURBS_SAMPLES_PER_KNOT_SPAN);
}
#endif
