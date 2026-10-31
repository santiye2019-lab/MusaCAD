/* SPDX-License-Identifier: MIT
 * First-party 2D INSERT-instance transforms for caller-verified geometry.
 */
#ifndef MUSA_DWG1032_AFFINE2D_H
#define MUSA_DWG1032_AFFINE2D_H
#include "musa_dwg1032_point.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Row-major affine map: x'=xx*x+xy*y+tx; y'=yx*x+yy*y+ty.
 * No DWG bytes are decoded; the caller must verify INSERT parameters,
 * units, extrusion and OCS/UCS before using this planar map. */
typedef struct MusaDwg1032Affine2 {
    double xx, xy, tx, yx, yy, ty;
} MusaDwg1032Affine2;
/* Build T(insertion)*R(angle)*S(sx,sy), in radians. Reject zero scales,
 * nonfinite parameters, overflow and aliased output. Output is unchanged
 * on failure. Reflections (negative scale) are supported. */
int musa_dwg1032_affine2_insert(const MusaDwg1032Point2 *insertion,
    double sx, double sy, double radians, MusaDwg1032Affine2 *out);
/* Compose outer(inner(point)) for a particular nested INSERT occurrence.
 * The output must not alias either input; fail atomically. */
int musa_dwg1032_affine2_compose(const MusaDwg1032Affine2 *outer,
    const MusaDwg1032Affine2 *inner, MusaDwg1032Affine2 *out);
/* Transform a batch without allocation. Input/output must be disjoint from
 * one another and the transform. Reject nonfinite input or output, overflow,
 * and invalid transforms. Preflight all points before writing ANY result.
 * O(count) time, O(1) memory. */
int musa_dwg1032_affine2_apply(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Point2 *points, size_t count, MusaDwg1032Point2 *out);
#ifdef __cplusplus
}
#endif
#endif
