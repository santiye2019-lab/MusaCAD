/* SPDX-License-Identifier: MIT
 * Independent planar array-INSERT placement for predecoded CAD fields.
 */
#ifndef MUSA_SCENE2D_ARRAY_H
#define MUSA_SCENE2D_ARRAY_H
#include "musa_scene2d_insert.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaScene2dInsertArray {
    MusaScene2dInsertPlacement placement;
    size_t columns, rows;
    double column_spacing, row_spacing; /* Local block axes, before scale/rotation. */
} MusaScene2dInsertArray;
/* Compute the affine transform for one instance of an already-decoded
 * rectangular INSERT array, with columns varying along local +X and rows
 * along local +Y. Array offsets are scaled and rotated with the block.
 * All fields/results must be finite, dimensions positive, indices in range.
 * Input and output may not overlap. Failure leaves out[0..5] unchanged.
 * Does not parse MINSERT, OCS, extrusion, or any AC1032 DWG bytes. */
int musa_scene2d_array_affine(const MusaScene2dInsertArray *array,
                               size_t column, size_t row, double out[6]);
#ifdef __cplusplus
}
#endif
#endif
