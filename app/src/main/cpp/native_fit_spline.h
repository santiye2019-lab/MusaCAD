/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_NATIVE_FIT_SPLINE_H
#define MUSACAD_NATIVE_FIT_SPLINE_H
/*
 * Read-only CAD-style natural cubic interpolation through DWG SPLINE fit
 * points when scenario=2 has no control points or knots.
 *
 * Chord-length parameterization, natural zero-second-derivative endpoints,
 * tridiagonal O(n) solving. This evaluates the same mathematical curve as
 * natural CAD cubic B-spline fit-point interpolation (e.g. BricsCAD-like).
 *
 * Explicit endpoint tangents and non-chord settings MUST be checked by the
 * caller; this helper does NOT pretend it can reconstruct all AutoCAD fits.
 * Reject non-finite/duplicate/ill-conditioned fit points rather than draw a
 * misleading control polygon.
 */
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#define MUSA_CAD_FIT_MAX_POINTS 512
#define MUSA_CAD_FIT_SAMPLES_PER_SPAN 12

typedef struct {
    double x,y,z;
} MusaCadFitPoint;

typedef struct {
    double x,y,t,mx,my;  /* fit point, cumulative parameter, 2nd derivatives */
} MusaCadNaturalKnot;

static int musa_cad_fit_natural_build(
        const MusaCadFitPoint *fit,size_t n,MusaCadNaturalKnot *out){
    if(!fit||!out||n<3||n>MUSA_CAD_FIT_MAX_POINTS)return 0;
    out[0]=(MusaCadNaturalKnot){fit[0].x,fit[0].y,0.,0.,0.};
    if(!isfinite(fit[0].x)||!isfinite(fit[0].y)||
       !isfinite(fit[0].z))return 0;
    for(size_t i=1;i<n;i++){
        if(!isfinite(fit[i].x)||!isfinite(fit[i].y)||
           !isfinite(fit[i].z))return 0;
        double dx=fit[i].x-fit[i-1].x;
        double dy=fit[i].y-fit[i-1].y;
        double dz=fit[i].z-fit[i-1].z;
        double length=hypot(hypot(dx,dy),dz);
        if(!isfinite(length)||length<=0.)return 0;
        double distance=out[i-1].t+length;
        if(!isfinite(distance)||distance<=out[i-1].t)return 0;
        out[i]=(MusaCadNaturalKnot){fit[i].x,fit[i].y,distance,0.,0.};
    }
    double total=out[n-1].t;
    if(!isfinite(total)||total<=0.)return 0;
    for(size_t i=1;i<n;i++){
        out[i].t/=total;
        if(!isfinite(out[i].t)||out[i].t<=out[i-1].t)
            return 0;
    }
    out[n-1].t=1.;
    /* Thomas tridiagonal solve for natural cubic second derivatives:
     * h_prev*M[i-1] + 2*(h_prev+h_next)*M[i] + h_next*M[i+1] =
     *    6*(slope_next-slope_prev).
     */
    double upper[MUSA_CAD_FIT_MAX_POINTS]={0.};
    for(size_t i=1;i+1<n;i++){
        double a=out[i].t-out[i-1].t,b=out[i+1].t-out[i].t;
        if(a<1e-12||b<1e-12)return 0;
        double diag=2*(a+b)-a*upper[i-1];
        if(!isfinite(diag)||diag<=0.)return 0;
        upper[i]=b/diag;
        double rx=6.*((out[i+1].x-out[i].x)/b-
                       (out[i].x-out[i-1].x)/a);
        double ry=6.*((out[i+1].y-out[i].y)/b-
                       (out[i].y-out[i-1].y)/a);
        out[i].mx=(rx-a*out[i-1].mx)/diag;
        out[i].my=(ry-a*out[i-1].my)/diag;
        if(!isfinite(out[i].mx)||!isfinite(out[i].my))return 0;
    }
    for(size_t i=n-2;i>=1;i--){
        out[i].mx-=upper[i]*out[i+1].mx;
        out[i].my-=upper[i]*out[i+1].my;
        if(!isfinite(out[i].mx)||!isfinite(out[i].my))return 0;
        if(i==1)break;
    }
    return 1;
}

/* CAD-style chord-length cubic fit with explicitly supplied endpoint
 * tangent directions. The tangent magnitudes are normalized to the total
 * source fit-point chord length, matching the common CAD global interpolation
 * convention. Clamped boundary derivatives keep the intended entry/exit
 * directions; this MUST NOT be replaced by natural endpoints.
 *
 * Equations at the endpoints:
 *   2*h0*M0 + h0*M1 = 6*(slope0 - tangent0)
 *   hlast*M[n-2] + 2*hlast*M[n-1] = 6*(tangentN - slopeLast)
 * Interiors use the natural cubic's tridiagonal equations.
 */
static int musa_cad_fit_clamped_build(
    const MusaCadFitPoint *fit,size_t n,
    MusaCadFitPoint start_tangent,MusaCadFitPoint end_tangent,
    MusaCadNaturalKnot *out){
    if(!musa_cad_fit_natural_build(fit,n,out))return 0;
    double begin_norm=hypot(hypot(start_tangent.x,start_tangent.y),
                            start_tangent.z);
    double end_norm=hypot(hypot(end_tangent.x,end_tangent.y),
                          end_tangent.z);
    if(!isfinite(begin_norm)||!isfinite(end_norm)||
       begin_norm<=0.||end_norm<=0.)return 0;
    double total=0.;
    for(size_t i=1;i<n;i++){
        double dx=fit[i].x-fit[i-1].x;
        double dy=fit[i].y-fit[i-1].y;
        double dz=fit[i].z-fit[i-1].z;
        total+=hypot(hypot(dx,dy),dz);
        if(!isfinite(total))return 0;
    }
    double v0x=(start_tangent.x/begin_norm)*total;
    double v0y=(start_tangent.y/begin_norm)*total;
    double vnx=(end_tangent.x/end_norm)*total;
    double vny=(end_tangent.y/end_norm)*total;
    if(!isfinite(v0x)||!isfinite(v0y)||!isfinite(vnx)||
       !isfinite(vny))return 0;
    double upper[MUSA_CAD_FIT_MAX_POINTS]={0.};
    double h0=out[1].t-out[0].t;
    if(h0<1e-12)return 0;
    double den0=2.*h0;
    upper[0]=h0/den0;
    out[0].mx=(6.*((out[1].x-out[0].x)/h0-v0x))/den0;
    out[0].my=(6.*((out[1].y-out[0].y)/h0-v0y))/den0;
    if(!isfinite(out[0].mx)||!isfinite(out[0].my))return 0;
    for(size_t i=1;i+1<n;i++){
        double a=out[i].t-out[i-1].t;
        double b=out[i+1].t-out[i].t;
        if(a<1e-12||b<1e-12)return 0;
        double diagonal=2.*(a+b)-a*upper[i-1];
        if(!isfinite(diagonal)||diagonal<=0.)return 0;
        upper[i]=b/diagonal;
        double rx=6.*((out[i+1].x-out[i].x)/b-
                       (out[i].x-out[i-1].x)/a);
        double ry=6.*((out[i+1].y-out[i].y)/b-
                       (out[i].y-out[i-1].y)/a);
        out[i].mx=(rx-a*out[i-1].mx)/diagonal;
        out[i].my=(ry-a*out[i-1].my)/diagonal;
        if(!isfinite(out[i].mx)||!isfinite(out[i].my))return 0;
    }
    double hn=out[n-1].t-out[n-2].t;
    if(hn<1e-12)return 0;
    double den=2.*hn-hn*upper[n-2];
    if(!isfinite(den)||den<=0.)return 0;
    double rx=6.*(vnx-(out[n-1].x-out[n-2].x)/hn);
    double ry=6.*(vny-(out[n-1].y-out[n-2].y)/hn);
    out[n-1].mx=(rx-hn*out[n-2].mx)/den;
    out[n-1].my=(ry-hn*out[n-2].my)/den;
    if(!isfinite(out[n-1].mx)||!isfinite(out[n-1].my))return 0;
    for(size_t i=n-1;i-->0;){
        out[i].mx-=upper[i]*out[i+1].mx;
        out[i].my-=upper[i]*out[i+1].my;
        if(!isfinite(out[i].mx)||!isfinite(out[i].my))return 0;
    }
    return 1;
}

static int musa_cad_fit_natural_eval(
        const MusaCadNaturalKnot *knots,size_t n,double u,
        double *outx,double *outy){
    if(!knots||!outx||!outy||n<3||
       n>MUSA_CAD_FIT_MAX_POINTS||!isfinite(u)||
       u<0.||u>1.)return 0;
    size_t lo=0,hi=n-1;
    while(lo+1<hi){
        size_t m=lo+(hi-lo)/2;
        if(knots[m].t<=u)lo=m;
        else hi=m;
    }
    if(lo>=n-1)return 0;
    double h=knots[lo+1].t-knots[lo].t;
    if(!isfinite(h)||h<=0.)return 0;
    double a=(knots[lo+1].t-u)/h,b=1.-a;
    double mx=((a*a*a-a)*knots[lo].mx+
               (b*b*b-b)*knots[lo+1].mx)*h*h/6.;
    double my=((a*a*a-a)*knots[lo].my+
               (b*b*b-b)*knots[lo+1].my)*h*h/6.;
    *outx=a*knots[lo].x+b*knots[lo+1].x+mx;
    *outy=a*knots[lo].y+b*knots[lo+1].y+my;
    return isfinite(*outx)&&isfinite(*outy);
}

static int musa_cad_fit_zero_tangent(double x,double y,double z){
    return isfinite(x)&&isfinite(y)&&isfinite(z)&&
           x==0.&&y==0.&&z==0.;
}
#endif
