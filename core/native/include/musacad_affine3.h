/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_AFFINE3_H
#define MUSACAD_CORE_AFFINE3_H
/*
 * Portable CAD 3D affine transforms for Android / Windows / iPhone.
 * Row-major 3x4 matrix [R|t]. Column-vector convention:
 *   WCS_point = M * [object_x,object_y,object_z,1].
 * Composition parent(child(point)) = parent * child.
 *
 * Handles 3D nested block transforms including translation, rotations,
 * nonuniform scales, shear and mirrors. Does not define UCS command/UI,
 * ACIS B-Rep, perspective camera, or modify existing Android rendering.
 * All functions fail closed on nonfinite, singular or overflow values;
 * output buffers are not modified on failure.
 */
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaCoreAffine3 {
    double m[12];
} MusaCoreAffine3;

static inline MusaCoreAffine3 musa_core_affine3_identity(void) {
    MusaCoreAffine3 a={{1,0,0,0, 0,1,0,0, 0,0,1,0}};
    return a;
}
static inline int musa_core_affine3_finite(const MusaCoreAffine3 *a) {
    if(!a)return 0;
    for(int i=0;i<12;i++)if(!isfinite(a->m[i]))return 0;
    return 1;
}
static inline int musa_core_affine3_compose(
        const MusaCoreAffine3 *parent,const MusaCoreAffine3 *child,
        MusaCoreAffine3 *out) {
    if(!out||!musa_core_affine3_finite(parent)||
       !musa_core_affine3_finite(child))return 0;
    MusaCoreAffine3 result={0};
    for(int r=0;r<3;r++) {
        for(int c=0;c<4;c++) {
            double v=c==3?parent->m[4*r+3]:0.;
            for(int k=0;k<3;k++)
                v+=parent->m[4*r+k]*child->m[4*k+c];
            if(!isfinite(v))return 0;
            result.m[4*r+c]=v;
        }
    }
    *out=result;
    return 1;
}
static inline int musa_core_affine3_map(
        const MusaCoreAffine3 *a,double x,double y,double z,
        double output[3],int direction_only) {
    if(!output||!musa_core_affine3_finite(a)||
       !isfinite(x)||!isfinite(y)||!isfinite(z))return 0;
    double mapped[3];
    for(int i=0;i<3;i++) {
        const int n=4*i;
        mapped[i]=a->m[n]*x+a->m[n+1]*y+a->m[n+2]*z+
                  (direction_only?0.:a->m[n+3]);
        if(!isfinite(mapped[i]))return 0;
    }
    output[0]=mapped[0];output[1]=mapped[1];output[2]=mapped[2];
    return 1;
}
static inline int musa_core_affine3_map_point(
        const MusaCoreAffine3 *a,double x,double y,double z,double output[3]) {
    return musa_core_affine3_map(a,x,y,z,output,0);
}
static inline int musa_core_affine3_map_vector(
        const MusaCoreAffine3 *a,double x,double y,double z,double output[3]) {
    return musa_core_affine3_map(a,x,y,z,output,1);
}
static inline int musa_core_affine3_inverse(
        const MusaCoreAffine3 *a,MusaCoreAffine3 *out) {
    if(!out||!musa_core_affine3_finite(a))return 0;
    /* Scaled partial pivoting avoids rejecting valid nonuniform scales just
     * because one axis uses much smaller drawing units than another.
     */
    double aug[3][6]={{0}};
    double row_scale[3]={0};
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            aug[i][j]=a->m[4*i+j];
            double magnitude=fabs(aug[i][j]);
            if(magnitude>row_scale[i])row_scale[i]=magnitude;
        }
        if(row_scale[i]==0.)return 0;
        aug[i][i+3]=1.;
    }
    for(int c=0;c<3;c++){
        int pivot=c;double ratio=-1.;
        for(int r=c;r<3;r++){
            double score=fabs(aug[r][c])/row_scale[r];
            if(score>ratio){ratio=score;pivot=r;}
        }
        if(!isfinite(ratio)||ratio<=1e-13)return 0;
        if(pivot!=c){
            for(int j=0;j<6;j++){
                double tmp=aug[c][j];aug[c][j]=aug[pivot][j];
                aug[pivot][j]=tmp;
            }
            double tmp=row_scale[c];row_scale[c]=row_scale[pivot];
            row_scale[pivot]=tmp;
        }
        double scale=aug[c][c];
        if(!isfinite(scale)||scale==0.)return 0;
        for(int j=0;j<6;j++){
            aug[c][j]/=scale;
            if(!isfinite(aug[c][j]))return 0;
        }
        for(int i=0;i<3;i++)if(i!=c){
            double factor=aug[i][c];
            for(int j=0;j<6;j++){
                aug[i][j]-=factor*aug[c][j];
                if(!isfinite(aug[i][j]))return 0;
            }
        }
    }
    MusaCoreAffine3 inv={0};
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++)inv.m[4*i+j]=aug[i][3+j];
        double translation=0.;
        for(int k=0;k<3;k++)
            translation-=inv.m[4*i+k]*a->m[4*k+3];
        if(!isfinite(translation))return 0;
        inv.m[4*i+3]=translation;
    }
    *out=inv;
    return 1;
}
static inline int musa_core_affine3_determinant(
        const MusaCoreAffine3 *a,double *out){
    if(!out||!musa_core_affine3_finite(a))return 0;
    const double *m=a->m;
    double value=m[0]*(m[5]*m[10]-m[6]*m[9])-
                 m[1]*(m[4]*m[10]-m[6]*m[8])+
                 m[2]*(m[4]*m[9]-m[5]*m[8]);
    if(!isfinite(value))return 0;
    *out=value;return 1;
}
#ifdef __cplusplus
}
#endif
#endif
