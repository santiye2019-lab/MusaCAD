/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_AFFINE_H
#define MUSACAD_CORE_AFFINE_H
/*
 * One C11 implementation of MusaCAD's 2D WCS affine geometry for Android,
 * Windows and iOS. Used by DWG INSERT, MINSERT and dimension block expansion.
 * Coordinates are doubles: conversion to float stream happens later.
 *
 * Matrix is [a c tx; b d ty; 0 0 1]. Composition parent*child maps a
 * child-local point into the parent's WCS without any platform UI state.
 */
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaCoreAffine2 {
    double a,b,c,d,tx,ty;
} MusaCoreAffine2;

static inline MusaCoreAffine2 musa_core_affine_identity(void) {
    const MusaCoreAffine2 m={1,0,0,1,0,0};
    return m;
}

/* Retains the production native_scene.c evaluation order exactly. */
static inline MusaCoreAffine2 musa_core_affine_multiply(
    MusaCoreAffine2 p,MusaCoreAffine2 q) {
    MusaCoreAffine2 r;
    r.a=p.a*q.a+p.c*q.b;r.b=p.b*q.a+p.d*q.b;
    r.c=p.a*q.c+p.c*q.d;r.d=p.b*q.c+p.d*q.d;
    r.tx=p.a*q.tx+p.c*q.ty+p.tx;r.ty=p.b*q.tx+p.d*q.ty+p.ty;
    return r;
}

/* An exact double WCS transform. Existing callers validate float range later. */
static inline void musa_core_affine_map(
    MusaCoreAffine2 m,double x,double y,double *ox,double *oy) {
    *ox=m.a*x+m.c*y+m.tx;
    *oy=m.b*x+m.d*y+m.ty;
}

/*
 * New safe reverse selection transform. Does not alter output on failure.
 * A determinant close to zero relative to linear scale is not invertible for
 * reliable CAD hit-testing. Excludes non-finite matrices and results.
 */
static inline int musa_core_affine_inverse(
    MusaCoreAffine2 m,MusaCoreAffine2 *out) {
    if(!out||!isfinite(m.a)||!isfinite(m.b)||!isfinite(m.c)||
       !isfinite(m.d)||!isfinite(m.tx)||!isfinite(m.ty))return 0;
    double scale=fmax(fmax(fabs(m.a),fabs(m.b)),
                      fmax(fabs(m.c),fabs(m.d)));
    if(!(scale>0)||!isfinite(scale))return 0;
    double a=m.a/scale,b=m.b/scale,c=m.c/scale,d=m.d/scale;
    double det=a*d-b*c;
    if(!isfinite(det)||fabs(det)<1e-12)return 0;
    /* Normalization reduces premature overflow from large scale factors. */
    double ia=(d/det)/scale,ib=(-b/det)/scale;
    double ic=(-c/det)/scale,id=(a/det)/scale;
    MusaCoreAffine2 result={ia,ib,ic,id,
        -(ia*m.tx+ic*m.ty),-(ib*m.tx+id*m.ty)};
    if(!isfinite(result.a)||!isfinite(result.b)||
       !isfinite(result.c)||!isfinite(result.d)||
       !isfinite(result.tx)||!isfinite(result.ty))return 0;
    *out=result;
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
