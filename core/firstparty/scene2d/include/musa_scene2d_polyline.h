/* SPDX-License-Identifier: MIT
 * Independently authored bounds for decoded 2D polyline bulge segments.
 */
#ifndef MUSA_SCENE2D_POLYLINE_H
#define MUSA_SCENE2D_POLYLINE_H
#include "musa_scene2d_geometry.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bulge is tan(signed included angle / 4) for the segment starting
 * at this vertex. Positive bulge traverses counterclockwise in XY.
 * The final vertex's bulge is used only when closed == 1.
 * Coordinates and bulges must be finite; count >= 2; closed is 0 or 1.
 * Bounds are conservative and include circular-arc extrema. This helper
 * accepts decoded vertex data; it does NOT parse or interpret DWG bytes.
 * Returns 0 without modifying *out on malformed/overflowing input.
 * No allocations or external CAD dependencies.
 */
typedef struct {
    double x, y, bulge;
} MusaScene2dPolylineVertex;

int musa_scene2d_polyline_bounds(const MusaScene2dPolylineVertex *vertices,
                                size_t count, int closed, MusaScene2dBox *out);
#ifdef __cplusplus
}
#endif
#endif
