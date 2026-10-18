/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_face_light.h"
#include <math.h>
#include <stdint.h>
double musa_core_face_lambert_f32(const float *xyz,size_t count,
                                  int a,int b,int c){
    if(!xyz||count>SIZE_MAX/3u||a<0||b<0||c<0||
       (size_t)a>=count||(size_t)b>=count||(size_t)c>=count)return -1.0;
    const size_t i=3u*(size_t)a,j=3u*(size_t)b,k=3u*(size_t)c;
    for(int d=0;d<3;d++){
        if(!isfinite(xyz[i+d])||!isfinite(xyz[j+d])||
           !isfinite(xyz[k+d]))return -1.0;
    }
    const double ux=(double)xyz[j]-xyz[i];
    const double uy=(double)xyz[j+1]-xyz[i+1];
    const double uz=(double)xyz[j+2]-xyz[i+2];
    const double vx=(double)xyz[k]-xyz[i];
    const double vy=(double)xyz[k+1]-xyz[i+1];
    const double vz=(double)xyz[k+2]-xyz[i+2];
    const double nx=uy*vz-uz*vy;
    const double ny=uz*vx-ux*vz;
    const double nz=ux*vy-uy*vx;
    const double len=sqrt(nx*nx+ny*ny+nz*nz);
    if(!isfinite(len)||len<=0.0)return -1.0;
    const double light=fabs((0.35*nx-0.4*ny+0.8*nz)/len);
    if(!isfinite(light))return -1.0;
    return fmin(1.0,fmax(0.0,light));
}
