/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_FACE_LIGHT_H
#define MUSACAD_FACE_LIGHT_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Face-normal Lambert viewport intensity from indexed FLOAT32 XYZ.
 * Cross product is evaluated in double precision for huge/tiny CAD geometry.
 * Return 0..1 for a real triangle, -1 for malformed, nonfinite or degenerate
 * geometry. This is display-only, NOT proof of ACIS/B-Rep solidity.
 */
double musa_core_face_lambert_f32(const float *xyz,size_t point_count,
                                  int a,int b,int c);
#ifdef __cplusplus
}
#endif
#endif
