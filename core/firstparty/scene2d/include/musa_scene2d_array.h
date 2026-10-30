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
/* Generate every instance in row-major order, columns varying fastest.
 * Capacity is measured in affine transforms, not doubles; out has room for
 * 6*capacity doubles. Uses two validation passes and no heap allocation.
 * On failure, out and *written are unchanged. Inputs remain immutable.
 * Rejects dimension/count overflow and output/input aliasing. */
int musa_scene2d_array_fill(const MusaScene2dInsertArray *array,
                             double *out, size_t capacity, size_t *written);

/* Produce a bounded page of row-major INSERT-array transforms without
 * allocating or visiting preceding instances. start is a zero-based instance
 * index; indices >= columns*rows return an empty page. capacity is in
 * transforms, not doubles. out may be NULL only for an empty page.
 * A zero-capacity query returns the total count without writing transforms.
 * All requested instances are validated before any output is modified.
 * On failure, out, *written and *total remain unchanged; buffers must be
 * disjoint. This accepts already decoded planar fields, not DWG bytes. */
int musa_scene2d_array_fill_page(const MusaScene2dInsertArray *array,
                                  size_t start, double *out, size_t capacity,
                                  size_t *written, size_t *total);
#ifdef __cplusplus
}
#endif
#endif
