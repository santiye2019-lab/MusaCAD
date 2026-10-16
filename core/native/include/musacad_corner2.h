/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORNER2_H
#define MUSACAD_CORNER2_H
/*
 * Portable finite LINE corner primitive, common Android/Windows/iOS C core.
 * For a FILLET, parameter is radius; for CHAMFER, a symmetric setback.
 *
 * This C reference and MusaAiCadGeometry.cornerLines share a scenario suite;
 * JNI/UI integration is not claimed until native callers use this entry point.
 * No source object is modified on failure. No line extensions permitted.
 */
#include "musacad_line2.h"
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaCoreCorner2 {
    double ax,ay,atx,aty;
    double bx,by,btx,bty;
    double midx,midy;
} MusaCoreCorner2;

static inline int musa_core_corner2_trim(
    const double a[4],const double b[4],
    double pickax,double pickay,double pickbx,double pickby,
    double value,int fillet,MusaCoreCorner2 *out){
    if(!a||!b||!out||!isfinite(value)||!(value>1e-12)||
       !isfinite(pickax)||!isfinite(pickay)||!isfinite(pickbx)||!isfinite(pickby)||
       fabs(pickax)>1e12||fabs(pickay)>1e12||
       fabs(pickbx)>1e12||fabs(pickby)>1e12)return 0;
    MusaCoreLine2Hit hit;
    if(!musa_core_line2_intersect(a[0],a[1],a[2],a[3],
                                  b[0],b[1],b[2],b[3],&hit)||
       hit.t<0||hit.t>1||hit.u<0||hit.u>1)return 0;
    double dax=a[2]-a[0],day=a[3]-a[1];
    double dbx=b[2]-b[0],dby=b[3]-b[1];
    double lenA=hypot(dax,day),lenB=hypot(dbx,dby);
    if(!(lenA>1e-12)||!(lenB>1e-12))return 0;
    double pa=((pickax-a[0])*(dax/lenA)+(pickay-a[1])*(day/lenA))/lenA;
    double pb=((pickbx-b[0])*(dbx/lenB)+(pickby-b[1])*(dby/lenB))/lenB;
    if(!isfinite(pa)||!isfinite(pb)||
       fabs(pa-hit.t)<=1e-10||fabs(pb-hit.u)<=1e-10)return 0;
    int ea=pa<hit.t?0:2,eb=pb<hit.u?0:2;
    double ax=a[ea],ay=a[ea+1],bx=b[eb],by=b[eb+1];
    double la=hypot(ax-hit.x,ay-hit.y),lb=hypot(bx-hit.x,by-hit.y);
    if(!(la>1e-12)||!(lb>1e-12)||!isfinite(la)||!isfinite(lb))return 0;
    double uax=(ax-hit.x)/la,uay=(ay-hit.y)/la;
    double ubx=(bx-hit.x)/lb,uby=(by-hit.y)/lb;
    double dot=fmax(-1.0,fmin(1.0,uax*ubx+uay*uby));
    double theta=acos(dot);
    const double pi=3.14159265358979323846;
    if(!(theta>pi/180.0)||!(theta<179.0*pi/180.0))return 0;
    double distance=fillet?value/tan(theta*0.5):value;
    if(!(distance>1e-12)||!isfinite(distance)||
       !(la-distance>fmax(1e-12,la*1e-10))||
       !(lb-distance>fmax(1e-12,lb*1e-10)))return 0;
    MusaCoreCorner2 result={0};
    result.ax=ax;result.ay=ay;result.bx=bx;result.by=by;
    result.atx=hit.x+uax*distance;result.aty=hit.y+uay*distance;
    result.btx=hit.x+ubx*distance;result.bty=hit.y+uby*distance;
    result.midx=(result.atx+result.btx)*0.5;
    result.midy=(result.aty+result.bty)*0.5;
    if(fillet){
        double bisx=uax+ubx,bisy=uay+uby,bis=hypot(bisx,bisy);
        double sine=sin(theta*0.5);
        if(!(bis>1e-12)||!(sine>1e-12))return 0;
        double cd=value/sine;
        double cx=hit.x+bisx/bis*cd,cy=hit.y+bisy/bis*cd;
        double back=hypot(hit.x-cx,hit.y-cy);
        if(!(back>1e-12)||!isfinite(back))return 0;
        result.midx=cx+(hit.x-cx)/back*value;
        result.midy=cy+(hit.y-cy)/back*value;
    }
    const double *v=&result.ax;
    for(int i=0;i<10;i++)
        if(!isfinite(v[i])||fabs(v[i])>1e12)return 0;
    *out=result;
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
