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
 * Compact float stream:
 * header = [1, primitiveCount, minX, minY, maxX, maxY]
 * LINE   = [1, aci, x1, y1, x2, y2]
 * POLY   = [2, aci, closed(0/1), pointCount, x1, y1, ...]
 * POINT  = [3, aci, x, y]
 */
int musa_scene_build(Dwg_Data *dwg,MusaNativeScene *scene);
void musa_scene_free(MusaNativeScene *scene);

#endif
