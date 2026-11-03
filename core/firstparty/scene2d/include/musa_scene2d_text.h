/* SPDX-License-Identifier: MIT
 * Independent UTF-8 validation and font-metric-based 2D text bounds.
 */
#ifndef MUSA_SCENE2D_TEXT_H
#define MUSA_SCENE2D_TEXT_H
#include "musa_scene2d_cull.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Validates strict UTF-8 Unicode scalar values. Embedded NUL is rejected
 * to prevent truncated display strings. Empty input is valid. On failure
 * *codepoints is unchanged. No DWG text-record or formatting decoding. */
int musa_scene2d_utf8_count(const uint8_t *bytes, size_t length,
                            size_t *codepoints);
/* Glyph metrics must come from a separately vetted font/shaping engine.
 * Each glyph's ink box is in its own pen-relative text coordinates; advance
 * moves the pen for the following glyph. Spaces may have no ink.
 * text_affine is [a,b,c,d,tx,ty], applied to all four ink-box corners.
 * Glyph count need not equal Unicode scalar count (ligatures/combining).
 * Returns conservative bounds for runs containing ink, without allocation.
 * All failures preserve *out; this does NOT render or decode DWG TEXT/MTEXT. */
typedef struct MusaScene2dTextGlyph {
    MusaScene2dBox ink;
    double advance_x, advance_y;
    uint8_t has_ink; /* exactly 0 or 1 */
} MusaScene2dTextGlyph;
int musa_scene2d_text_run_bounds(const MusaScene2dTextGlyph *glyphs,
                                 size_t count, const double text_affine[6],
                                 MusaScene2dBox *out);
#ifdef __cplusplus
}
#endif
#endif
