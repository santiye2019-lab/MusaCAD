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
 * Compact float stream, v4:
 * header = [4, primitiveCount, minX, minY, maxX, maxY, truncated]
 * ACI metadata      = [type*256 + ACI]
 * TrueColor metadata= [-(type*256+1), RGB24]
 * LINE   = [metadata..., x1, y1, x2, y2]
 * POLY   = [metadata..., signedPointCount, x1, y1, ...] (negative count = closed)
 * POINT  = [metadata..., x, y]
 * CURVE  = [metadata..., cx, cy, ux, uy, vx, vy, startRadians, sweepRadians]
 *          point(t) = center + U*cos(t) + V*sin(t)
 * Java keeps backward parsing for v1/v2/v3 scenes.
 */
int musa_scene_build(Dwg_Data *dwg,MusaNativeScene *scene);
void musa_scene_free(MusaNativeScene *scene);

#endif
