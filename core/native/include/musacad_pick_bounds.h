/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_PICK_BOUNDS_H
#define MUSACAD_PICK_BOUNDS_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* All platforms share conservative 2D pre-pick, never exact hit testing.
 * 0: vertex bounds, 1: circle, 2: arc, 3: unbounded/rotated ellipse.
 * An unknown or invalid envelope MUST return 1 (possible hit).
 * Return 0 only when its finite conservative envelope excludes the point. */
int musa_core_pick_bounds_f32(const float *xy,size_t float_count,int kind,
                              float x,float y,float tolerance);
#ifdef __cplusplus
}
#endif
#endif
