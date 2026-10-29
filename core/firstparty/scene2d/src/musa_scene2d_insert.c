/* SPDX-License-Identifier: MIT
 * Independently authored planar block INSERT transform (2026-10-29).
 */
#include "musa_scene2d_insert.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int overlap(const void *a, size_t na, const void *b, size_t nb) {
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < na : x - y < nb;
}
int musa_scene2d_insert_affine(const MusaScene2dInsertPlacement *placement,
                               double out[6]) {
    if (!placement || !out ||
        overlap(placement, sizeof(*placement), out, 6u*sizeof(*out))) return 0;
    const double fields[] = {
        placement->insertion_x, placement->insertion_y,
        placement->base_x, placement->base_y,
        placement->scale_x, placement->scale_y,
        placement->rotation_rad
    };
    for (size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i)
        if (!isfinite(fields[i])) return 0;
    const double c = cos(placement->rotation_rad);
    const double s = sin(placement->rotation_rad);
    const double a = c * placement->scale_x;
    const double b = s * placement->scale_x;
    const double cc = -s * placement->scale_y;
    const double d = c * placement->scale_y;
    const double tx = placement->insertion_x - a*placement->base_x - cc*placement->base_y;
    const double ty = placement->insertion_y - b*placement->base_x - d*placement->base_y;
    const double candidate[6] = {a,b,cc,d,tx,ty};
    for (size_t i = 0; i < 6; ++i)
        if (!isfinite(candidate[i])) return 0;
    for (size_t i = 0; i < 6; ++i) out[i] = candidate[i];
    return 1;
}
