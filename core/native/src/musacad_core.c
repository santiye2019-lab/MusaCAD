/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_core.h"
#include "musacad_nurbs_internal.h"

/*
 * One portable implementation shared by Android JNI's native_scene.c,
 * Windows desktop C ABI adapters, and iOS's Swift/C interop layer.
 * No platform-specific objects cross this boundary.
 */
unsigned int musa_core_abi_version(void) { return MUSA_CORE_ABI_VERSION; }

double musa_core_effective_weight(double weight,int explicit_weights) {
    return musa_nurbs_weight(weight,explicit_weights);
}
int musa_core_nurbs_validate(const MusaCoreNurbsControl *controls,size_t n,
                            const double *knots,size_t nk,int degree) {
    return musa_nurbs_valid(controls,n,knots,nk,degree);
}
int musa_core_nurbs_evaluate(const MusaCoreNurbsControl *controls,size_t n,
                            const double *knots,int degree,double u,
                            double *x,double *y) {
    /* Reject malformed definitions even if caller bypassed validation. */
    if(!controls||!knots||!x||!y||n<(size_t)(degree+1)||
       degree<1||degree>MUSA_NURBS_MAX_DEGREE)return 0;
    if(!musa_nurbs_valid(controls,n,knots,n+(size_t)degree+1u,degree))return 0;
    return musa_nurbs_eval(controls,n,knots,degree,u,x,y);
}
int musa_core_nurbs_preview_points(const double *knots,size_t n,int degree) {
    return musa_nurbs_preview_points(knots,n,degree);
}
