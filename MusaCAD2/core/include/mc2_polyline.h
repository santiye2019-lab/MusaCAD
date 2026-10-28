#ifndef MC2_POLYLINE_H
#define MC2_POLYLINE_H
#include "mc2_geometry.h"
#include "mc2_transform.h"
#include <stddef.h>
/* Vertices are connected in order; closed connects last to first. */
int mc2_polyline_bounds(const Mc2Point2 *vertices,size_t count,int closed,
                        const Mc2Affine2 *transform,Mc2Bounds2 *out);
double mc2_polyline_length(const Mc2Point2 *vertices,size_t count,int closed);
#endif
