/* SPDX-License-Identifier: MIT
 * Independent affine placement for already-decoded planar CAD text.
 */
#ifndef MUSA_SCENE2D_TEXT_PLACEMENT_H
#define MUSA_SCENE2D_TEXT_PLACEMENT_H
#ifdef __cplusplus
extern "C" {
#endif
/* Glyph coordinates are normalized to unit text height. The caller must
 * supply the measured alignment anchor in those same coordinates, after
 * shaping and justification. Negative width_factor mirrors horizontally.
 * This does not parse TEXT/MTEXT, select fonts, or perform OCS conversion. */
typedef struct MusaScene2dTextPlacement {
    double insertion_x, insertion_y;
    double height, width_factor;
    double oblique_rad, rotation_rad;
    double anchor_x, anchor_y;
} MusaScene2dTextPlacement;
/* p_world = insertion + R(rotation) * [height*width_factor,
 * height*tan(oblique); 0, height] * (p_glyph - anchor).
 * Returns {a,b,c,d,tx,ty}; rejects nonfinite/degenerate coefficients and
 * overlapping input/output without modifying out on failure. */
int musa_scene2d_text_affine(const MusaScene2dTextPlacement *placement,
                             double out[6]);
#ifdef __cplusplus
}
#endif
#endif
