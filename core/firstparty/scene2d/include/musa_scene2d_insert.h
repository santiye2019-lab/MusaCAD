/* SPDX-License-Identifier: MIT
 * Independent affine adapter for already-decoded planar CAD INSERT fields.
 */
#ifndef MUSA_SCENE2D_INSERT_H
#define MUSA_SCENE2D_INSERT_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* All coordinates are in the same planar coordinate system. Extrusion,
 * OCS-to-WCS conversion and arbitrary 3D block placement are NOT handled.
 * The insertion point is where the block's base point must land.
 * Negative scales mirror the block; zero scales produce degenerate geometry.
 */
typedef struct MusaScene2dInsertPlacement {
    double insertion_x, insertion_y;
    double base_x, base_y;
    double scale_x, scale_y;
    double rotation_rad;
} MusaScene2dInsertPlacement;
/* Produces {a,b,c,d,tx,ty} for the MusaScene2dNode affine field:
 * p_world = insertion + R(rotation)*S(scale)*(p_block - base).
 * All input and resulting coefficients must be finite. The source and
 * destination must not overlap. On failure out is unchanged.
 * No DWG record decoding or 3D extrusion interpretation is performed.
 */
int musa_scene2d_insert_affine(const MusaScene2dInsertPlacement *placement,
                               double out[6]);
/* Convert an array of independent, already-decoded INSERT placements.
 * capacity counts six-double transforms. Validates the complete input
 * before publishing any result, so failure leaves all out entries intact.
 * No allocation, O(count) time, source/output must be disjoint.
 * Empty input succeeds with NULL buffers. Not a DWG entity decoder. */
int musa_scene2d_insert_affine_batch(
    const MusaScene2dInsertPlacement *placements, size_t count,
    double (*out)[6], size_t capacity);
/* Read only the requested page of arbitrary decoded INSERT placements.
 * start >= count returns an empty page; capacity is in transforms.
 * An empty page permits NULL out. The input array must remain immutable.
 * Validates the requested page before writing, O(page_count) time with
 * no allocation. Failure leaves out, *written and *total unchanged.
 * A zero-capacity call returns total without examining any placement.
 * This is not DWG framing, ownership or extrusion interpretation. */
int musa_scene2d_insert_affine_page(
    const MusaScene2dInsertPlacement *placements, size_t count,
    size_t start, double (*out)[6], size_t capacity,
    size_t *written, size_t *total);
#ifdef __cplusplus
}
#endif
#endif
