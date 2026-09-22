/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_NATIVE_SCENE_H
#define MUSACAD_NATIVE_SCENE_H
#include <stddef.h>
#include "dwg.h"

typedef struct {
    float *values;
    size_t count;
    size_t capacity;
    int primitives;
    double min_x,min_y,max_x,max_y;
    int has_bounds;
    int truncated;
} MusaNativeScene;

/*
 * Compact float stream, v3:
 * header = [3, primitiveCount, minX, minY, maxX, maxY, truncated]
 * code   = type*256 + ACI
 * LINE   = [code(1), x1, y1, x2, y2]
 * POLY   = [code(2), signedPointCount, x1, y1, ...] (negative count = closed)
 * POINT  = [code(3), x, y]
 * CURVE  = [code(4), cx, cy, ux, uy, vx, vy, startRadians, sweepRadians]
 *          point(t) = center + U*cos(t) + V*sin(t)
 * Java keeps backward parsing for v1/v2 scenes.
 */
int musa_scene_build(Dwg_Data *dwg,MusaNativeScene *scene);
void musa_scene_free(MusaNativeScene *scene);

#endif
