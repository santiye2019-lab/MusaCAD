/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_CORE_H
#define MUSACAD_CORE_H
/*
 * Stable C-facing API for reusable MusaCAD geometry. No JNI, Android SDK,
 * Win32 or Apple Foundation headers may appear in the core ABI.
 * ABI 1: append functions rather than changing existing struct layouts.
 */
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MUSA_CORE_ABI_VERSION 1u
#define MUSA_CORE_NURBS_MAX_DEGREE 16
#define MUSA_CORE_NURBS_MAX_CONTROL 4096
#define MUSA_CORE_NURBS_SAMPLES_PER_KNOT_SPAN 12
typedef struct MusaCoreNurbsControl {
    double x, y, w;
} MusaCoreNurbsControl;

/* Returns the ABI contract version so platform adapters can fail closed. */
unsigned int musa_core_abi_version(void);

/* LibreDWG may serialize implicit weights as zero: they mean 1, not 0. */
double musa_core_effective_weight(double stored_weight,int has_explicit_weights);

/* Return 1 on success; 0 on invalid, disconnected or out-of-bounds geometry. */
int musa_core_nurbs_validate(const MusaCoreNurbsControl *controls,size_t count,
                            const double *knots,size_t knot_count,int degree);
int musa_core_nurbs_evaluate(const MusaCoreNurbsControl *controls,size_t count,
                            const double *knots,size_t knot_count,int degree,double parameter,
                            double *out_x,double *out_y);
int musa_core_nurbs_preview_points(const double *knots,size_t control_count,
                                  size_t knot_count,
                                  int degree);
#ifdef __cplusplus
}
#endif
#endif
