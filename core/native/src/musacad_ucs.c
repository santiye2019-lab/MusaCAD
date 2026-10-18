/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_ucs.h"
#include <math.h>
#include <string.h>
#define UCS_EPS 1e-10
#define UCS_MAX_ORIGIN 1e9
static int finite3(const double v[3]){
    return v&&isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);
}
static double dot3(const double a[3],const double b[3]){
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static void cross3(const double a[3],const double b[3],double out[3]){
    out[0]=a[1]*b[2]-a[2]*b[1];
    out[1]=a[2]*b[0]-a[0]*b[2];
    out[2]=a[0]*b[1]-a[1]*b[0];
}
static int unit3(double a[3]){
    if(!finite3(a))return 0;
    double length=hypot(hypot(a[0],a[1]),a[2]);
    if(!isfinite(length)||length<UCS_EPS)return 0;
    for(int i=0;i<3;i++){
        a[i]/=length;
        if(!isfinite(a[i]))return 0;
    }
    return 1;
}
static int valid(const MusaCoreUcsFrame *frame){
    if(!frame||!finite3(frame->origin))return 0;
    for(int i=0;i<3;i++){
        if(!finite3(frame->axis[i]))return 0;
    }
    return 1;
}
int musa_core_ucs_create(const double in[9],MusaCoreUcsFrame *out){
    if(!in||!out)return 0;
    for(int i=0;i<9;i++)if(!isfinite(in[i]))return 0;
    MusaCoreUcsFrame next={0};
    for(int i=0;i<3;i++){
        if(fabs(in[i])>UCS_MAX_ORIGIN)return 0;
        next.origin[i]=in[i];
        next.axis[0][i]=in[3+i];
        next.axis[1][i]=in[6+i];
    }
    if(!unit3(next.axis[0]))return 0;
    double projection=dot3(next.axis[0],next.axis[1]);
    if(!isfinite(projection))return 0;
    for(int i=0;i<3;i++){
        next.axis[1][i]-=projection*next.axis[0][i];
    }
    if(!unit3(next.axis[1]))return 0;
    cross3(next.axis[0],next.axis[1],next.axis[2]);
    if(!unit3(next.axis[2]))return 0;
    *out=next;
    return 1;
}
int musa_core_ucs_to_world(const MusaCoreUcsFrame *ucs,
                           const double xyz[3],double out[3]){
    if(!out||!finite3(xyz)||!valid(ucs))return 0;
    double mapped[3];
    for(int i=0;i<3;i++){
        mapped[i]=ucs->origin[i]+xyz[0]*ucs->axis[0][i]
                +xyz[1]*ucs->axis[1][i]+xyz[2]*ucs->axis[2][i];
        if(!isfinite(mapped[i]))return 0;
    }
    memcpy(out,mapped,sizeof(mapped));return 1;
}
int musa_core_ucs_to_local(const MusaCoreUcsFrame *ucs,
                           const double xyz[3],double out[3]){
    if(!out||!finite3(xyz)||!valid(ucs))return 0;
    double delta[3];
    for(int i=0;i<3;i++){
        delta[i]=xyz[i]-ucs->origin[i];
        if(!isfinite(delta[i]))return 0;
    }
    double mapped[3];
    for(int i=0;i<3;i++){
        mapped[i]=dot3(delta,ucs->axis[i]);
        if(!isfinite(mapped[i]))return 0;
    }
    memcpy(out,mapped,sizeof(mapped));return 1;
}
int musa_core_ucs_move(const MusaCoreUcsFrame *ucs,int axis,
                       double distance,double out[3]){
    if(!out||!valid(ucs)||axis<0||axis>2||!isfinite(distance))return 0;
    double mapped[3];
    for(int i=0;i<3;i++){
        mapped[i]=distance*ucs->axis[axis][i];
        if(!isfinite(mapped[i]))return 0;
    }
    memcpy(out,mapped,sizeof(mapped));return 1;
}
