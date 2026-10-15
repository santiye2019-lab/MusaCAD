/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_SCENE_COORD_SAFETY_H
#define MUSACAD_SCENE_COORD_SAFETY_H

#include <float.h>
#include <math.h>

/*
 * Native scene v6 transports world geometry in IEEE-754 float words.
 * A finite double outside float range would become +/-Inf after conversion,
 * corrupting Android bounding boxes and potentially creating fake geometry.
 * Do not silently substitute (0,0) for a bad source coordinate.
 */
static inline int musa_scene_float_safe(double value) {
    return isfinite(value) && value <= (double)FLT_MAX &&
           value >= -(double)FLT_MAX;
}

static inline int musa_scene_xy_safe(double x, double y) {
    return musa_scene_float_safe(x) && musa_scene_float_safe(y);
}

#endif
