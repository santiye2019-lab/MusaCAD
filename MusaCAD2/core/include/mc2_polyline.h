#ifndef MC2_POLYLINE_H
#define MC2_POLYLINE_H
#include "mc2_geometry.h"
#include "mc2_transform.h"
#include <stddef.h>
/* Vertices are connected in order; closed connects last to first. */
int mc2_polyline_bounds(const Mc2Point2 *vertices,size_t count,int closed,
                        const Mc2Affine2 *transform,Mc2Bounds2 *out);
/* Signed shoelace area: positive for counterclockwise closed polygons. */
int mc2_polyline_signed_area(const Mc2Point2 *vertices,size_t count,
                             int closed,double *out_area);
double mc2_polyline_length(const Mc2Point2 *vertices,size_t count,int closed);
/* Bulge = tan(signed_sweep/4), attached to each start vertex.
 * bulges may be NULL for straight segments. The last bulge is used only
 * for a closed polyline. Failure leaves out_length unchanged. */
int mc2_polyline_bulge_length(const Mc2Point2 *vertices,
                              const double *bulges, size_t count,
                              int closed, double *out_length);
#endif
