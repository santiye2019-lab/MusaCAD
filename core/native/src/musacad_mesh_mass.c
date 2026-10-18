/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <math.h>
#include <stdint.h>

/*
 * Native implementation of the same reference-shifted tetrahedral volume
 * and compensated first-moment math as Android's Cad3dSignedVolume.
 * The caller MUST independently audit edge incidence, duplicate facets,
 * winding and degeneracy before setting topology_prevalidated=1. Merely
 * passing '1' does not establish a solid; this is NOT a B-Rep/ACIS kernel.
 */
static int add_compensated(double value,double *sum,double *correction){
    const double next=*sum+value;
    if(!isfinite(value)||!isfinite(next))return 0;
    if(fabs(*sum)>=fabs(value))*correction+=(*sum-next)+value;
    else *correction+=(value-next)+*sum;
    *sum=next;
    return isfinite(*correction);
}

int musa_core_mesh_mass_f32(const float *xyz,size_t point_count,
                            const int *triangles,size_t face_count,
                            int topology_prevalidated,
                            MusaCoreMeshMass *out){
    if(!out||!xyz||!triangles||topology_prevalidated!=1||
       !point_count||!face_count||face_count>4000000u||
       point_count>SIZE_MAX/3u||face_count>SIZE_MAX/3u)return 0;
    int ref=triangles[0];
    if(ref<0||(size_t)ref>=point_count)return 0;
    size_t origin=(size_t)ref*3u;
    double ox=xyz[origin],oy=xyz[origin+1],oz=xyz[origin+2];
    if(!isfinite(ox)||!isfinite(oy)||!isfinite(oz))return 0;

    double sum=0,fix=0,mx=0,my=0,mz=0,fx=0,fy=0,fz=0;
    for(size_t i=0;i<face_count;i++){
        size_t j=i*3u;
        int ia=triangles[j],ib=triangles[j+1],ic=triangles[j+2];
        if(ia<0||ib<0||ic<0||(size_t)ia>=point_count||
           (size_t)ib>=point_count||(size_t)ic>=point_count)return 0;
        size_t a=(size_t)ia*3u,b=(size_t)ib*3u,c=(size_t)ic*3u;
        double ax=(double)xyz[a]-ox,ay=(double)xyz[a+1]-oy,az=(double)xyz[a+2]-oz;
        double bx=(double)xyz[b]-ox,by=(double)xyz[b+1]-oy,bz=(double)xyz[b+2]-oz;
        double cx=(double)xyz[c]-ox,cy=(double)xyz[c+1]-oy,cz=(double)xyz[c+2]-oz;
        if(!isfinite(ax)||!isfinite(ay)||!isfinite(az)||
           !isfinite(bx)||!isfinite(by)||!isfinite(bz)||
           !isfinite(cx)||!isfinite(cy)||!isfinite(cz))return 0;
        double six=ax*(by*cz-bz*cy)+ay*(bz*cx-bx*cz)+az*(bx*cy-by*cx);
        double tetra=six/6.;
        if(!add_compensated(tetra,&sum,&fix))return 0;
        double tx=tetra*(ax+bx+cx)/4.;
        double ty=tetra*(ay+by+cy)/4.;
        double tz=tetra*(az+bz+cz)/4.;
        if(!add_compensated(tx,&mx,&fx)||
           !add_compensated(ty,&my,&fy)||
           !add_compensated(tz,&mz,&fz))return 0;
    }
    double signed_volume=sum+fix;
    if(!isfinite(signed_volume)||signed_volume==0.)return 0;
    double center_x=ox+(mx+fx)/signed_volume;
    double center_y=oy+(my+fy)/signed_volume;
    double center_z=oz+(mz+fz)/signed_volume;
    if(!isfinite(center_x)||!isfinite(center_y)||!isfinite(center_z))return 0;
    MusaCoreMeshMass result={signed_volume,center_x,center_y,center_z,face_count};
    *out=result;
    return 1;
}

/* All topology admission and mass semantics live in this portable core. */
int musa_core_mesh_mass_verified_f32(const float *xyz,size_t point_count,
                                     const int *triangles,size_t face_count,
                                     MusaCoreMeshMass *out){
    if(!xyz||!triangles||!out||!point_count||!face_count||
       point_count>1000000u||face_count>50000u||
       point_count>SIZE_MAX/3u||face_count>SIZE_MAX/3u)return 0;
    MusaCoreMeshTopology r={0};
    if(!musa_core_mesh_topology_f32(xyz,point_count,triangles,face_count,&r)||
       !r.checked||!r.closed_oriented_by_index||r.triangles!=face_count)
        return 0;
    return musa_core_mesh_mass_f32(xyz,point_count,triangles,face_count,1,out);
}
