/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include <math.h>
#include <stdint.h>

/*
 * One common CAD 3D ray/triangle selection kernel. Does not mutate geometry.
 * Uses double local WCS arithmetic for far-georeferenced CAD drawings and
 * avoids requiring platform-specific 3D math on Android/Windows/iPhone.
 */
#define MUSA_RAY_MAX_FACES 250000u
#define MUSA_RAY_MAX_POINTS 1000000u
static double dot3(const double a[3],const double b[3]) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static void cross3(const double a[3],const double b[3],double o[3]) {
    o[0]=a[1]*b[2]-a[2]*b[1];
    o[1]=a[2]*b[0]-a[0]*b[2];
    o[2]=a[0]*b[1]-a[1]*b[0];
}
static int valid3(const double a[3]) {
    return isfinite(a[0])&&isfinite(a[1])&&isfinite(a[2]);
}

int musa_core_mesh_ray_pick_f32(const float *xyz,size_t point_count,
                               const int *triangles,size_t face_count,
                               const double origin[3],const double direction[3],
                               double max_distance,MusaCoreMeshRayHit *out) {
    if(!out||!origin||!direction||!valid3(origin)||!valid3(direction)||
       !isfinite(max_distance)||max_distance<0.||
       point_count>MUSA_RAY_MAX_POINTS||face_count>MUSA_RAY_MAX_FACES||
       point_count>SIZE_MAX/3u||face_count>SIZE_MAX/3u||
       (face_count&&(!xyz||!triangles)))return -1;
    const double absmax=fmax(fabs(direction[0]),
                             fmax(fabs(direction[1]),fabs(direction[2])));
    if(absmax==0.)return -1;
    double d[3]={direction[0]/absmax,direction[1]/absmax,
                 direction[2]/absmax};
    const double norm=hypot(hypot(d[0],d[1]),d[2]);
    if(!isfinite(norm)||norm==0.)return -1;
    for(int i=0;i<3;i++)d[i]/=norm;
    if(!face_count)return 0;

    MusaCoreMeshRayHit best={0};
    int has_hit=0;
    /* Input is mesh-local floats; calculations widen before subtracting. */
    for(size_t f=0;f<face_count;f++) {
        const size_t b=f*3u;
        const int ix[3]={triangles[b],triangles[b+1],triangles[b+2]};
        for(int j=0;j<3;j++)
            if(ix[j]<0||(size_t)ix[j]>=point_count)return -1;
        double pts[3][3];
        for(int j=0;j<3;j++){
            const size_t k=(size_t)ix[j]*3u;
            for(int axis=0;axis<3;axis++){
                if(!isfinite(xyz[k+axis]))return -1;
                pts[j][axis]=xyz[k+axis];
            }
        }
        double e1[3],e2[3],from[3];
        for(int j=0;j<3;j++){
            e1[j]=pts[1][j]-pts[0][j];
            e2[j]=pts[2][j]-pts[0][j];
            from[j]=pts[0][j]-origin[j];
        }
        if(!valid3(e1)||!valid3(e2)||!valid3(from))return -1;
        double p[3];cross3(d,e2,p);
        const double det=dot3(e1,p);
        const double e1length=hypot(hypot(e1[0],e1[1]),e1[2]);
        const double e2length=hypot(hypot(e2[0],e2[1]),e2[2]);
        const double scale=e1length*e2length;
        if(!isfinite(det)||!isfinite(scale))return -1;
        if(scale==0.||fabs(det)<=1e-14*scale)continue;
        const double inv=1./det;
        const double u=dot3(from,p)*inv;
        if(!isfinite(u))return -1;
        if(u < -1e-12 || u > 1.+1e-12)continue;
        double q[3];cross3(from,e1,q);
        const double v=dot3(d,q)*inv;
        if(!isfinite(v))return -1;
        if(v < -1e-12 || u+v > 1.+1e-12)continue;
        const double t=dot3(e2,q)*inv;
        if(!isfinite(t))return -1;
        if(t<0.||t>max_distance)continue;
        if(has_hit&&t>=best.distance)continue;
        MusaCoreMeshRayHit candidate={0};
        candidate.triangle_index=f;
        candidate.distance=t;candidate.barycentric_u=u;
        candidate.barycentric_v=v;candidate.front_facing=det>0.;
        double wcs[3];
        for(int j=0;j<3;j++)wcs[j]=origin[j]+d[j]*t;
        if(!valid3(wcs))return -1;
        candidate.wcs_x=wcs[0];candidate.wcs_y=wcs[1];
        candidate.wcs_z=wcs[2];
        best=candidate;has_hit=1;
    }
    if(!has_hit)return 0;
    *out=best;return 1;
}
