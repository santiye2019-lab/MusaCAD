/* SPDX-License-Identifier: MIT
 * Independent affine adapter for already-decoded planar CAD INSERT fields.
 */
#ifndef MUSA_SCENE2D_INSERT_H
#define MUSA_SCENE2D_INSERT_H
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
#ifdef __cplusplus
}
#endif
#endif
