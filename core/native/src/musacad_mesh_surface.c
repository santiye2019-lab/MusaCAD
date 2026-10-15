/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <math.h>
#include <stdint.h>

/* 3D mesh SURFACE area/centroid, same compensated WCS arithmetic as the
 * already-shipped Java Cad3dSurfaceMetrics. This is an independent calculation
 * API; it is NOT an ACIS surface, exact analytic spline/B-Rep or metraj.
 */
static int compensated_add(double value,double *sum,double *correction){
    if(!isfinite(value))return 0;
    double next=*sum+value;
    if(!isfinite(next))return 0;
    if(fabs(*sum)>=fabs(value))*correction+=(*sum-next)+value;
    else *correction+=(value-next)+*sum;
    *sum=next;
    return isfinite(*correction);
}
int musa_core_mesh_surface_f32(const float *xyz,size_t point_count,
                              const int *triangles,size_t face_count,
                              MusaCoreMeshSurface *out){
    if(!out||point_count>1000000u||face_count>4000000u||
       point_count>SIZE_MAX/3u||face_count>SIZE_MAX/3u||
       (face_count&&!triangles)||(point_count&&!xyz))return 0;
    double ox=0,oy=0,oz=0;
    if(face_count){
        int first=triangles[0];
        if(first<0||(size_t)first>=point_count)return 0;
        size_t start=(size_t)first*3u;
        ox=xyz[start];oy=xyz[start+1];oz=xyz[start+2];
        if(!isfinite(ox)||!isfinite(oy)||!isfinite(oz))return 0;
    }
    double total=0,comp=0,mx=0,my=0,mz=0,comp_x=0,comp_y=0,comp_z=0;
    size_t degenerate=0;
    for(size_t i=0;i<face_count;i++){
        size_t t=i*3u;
        int ia=triangles[t],ib=triangles[t+1],ic=triangles[t+2];
        if(ia<0||ib<0||ic<0||(size_t)ia>=point_count||
           (size_t)ib>=point_count||(size_t)ic>=point_count)return 0;
        size_t a=(size_t)ia*3u,b=(size_t)ib*3u,c=(size_t)ic*3u;
        for(int k=0;k<3;k++)if(!isfinite(xyz[a+k])||
                                 !isfinite(xyz[b+k])||
                                 !isfinite(xyz[c+k]))return 0;
        double ux=(double)xyz[b]-xyz[a];
        double uy=(double)xyz[b+1]-xyz[a+1];
        double uz=(double)xyz[b+2]-xyz[a+2];
        double vx=(double)xyz[c]-xyz[a];
        double vy=(double)xyz[c+1]-xyz[a+1];
        double vz=(double)xyz[c+2]-xyz[a+2];
        double cross_x=uy*vz-uz*vy,cross_y=uz*vx-ux*vz,
               cross_z=ux*vy-uy*vx;
        double area=0.5*hypot(hypot(cross_x,cross_y),cross_z);
        if(!isfinite(area))return 0;
        if(area==0.)degenerate++;
        if(!compensated_add(area,&total,&comp))return 0;
        if(area>0.){
            double px=((double)xyz[a]-ox+(double)xyz[b]-ox+(double)xyz[c]-ox)/3.;
            double py=((double)xyz[a+1]-oy+(double)xyz[b+1]-oy+(double)xyz[c+1]-oy)/3.;
            double pz=((double)xyz[a+2]-oz+(double)xyz[b+2]-oz+(double)xyz[c+2]-oz)/3.;
            if(!compensated_add(area*px,&mx,&comp_x)||
               !compensated_add(area*py,&my,&comp_y)||
               !compensated_add(area*pz,&mz,&comp_z))return 0;
        }
    }
    double area=total+comp;
    if(!isfinite(area))return 0;
    MusaCoreMeshSurface result={0};
    result.area=area;
    result.triangle_count=face_count;result.degenerate_count=degenerate;
    result.centroid_x=NAN;result.centroid_y=NAN;result.centroid_z=NAN;
    if(area>0.){
        result.centroid_x=ox+(mx+comp_x)/area;
        result.centroid_y=oy+(my+comp_y)/area;
        result.centroid_z=oz+(mz+comp_z)/area;
        if(!isfinite(result.centroid_x)||!isfinite(result.centroid_y)||
           !isfinite(result.centroid_z))return 0;
    }
    *out=result;return 1;
}
