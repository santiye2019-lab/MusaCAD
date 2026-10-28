#ifndef MC2_SPATIAL_H
#define MC2_SPATIAL_H
#include "mc2_geometry.h"
#include "mc2_transform.h"
typedef struct { Mc2Bounds2 bounds; int valid; } Mc2SpatialBounds;
int mc2_bounds_intersects(const Mc2Bounds2 *a,const Mc2Bounds2 *b);
int mc2_transform_bounds(const Mc2Bounds2 *source,const Mc2Affine2 *transform,Mc2Bounds2 *out);
int mc2_entity_world_bounds(const Mc2Entity2 *entity,const Mc2Affine2 *world,Mc2Bounds2 *out);
#endif
