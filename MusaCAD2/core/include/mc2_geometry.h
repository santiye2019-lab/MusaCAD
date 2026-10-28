#ifndef MC2_GEOMETRY_H
#define MC2_GEOMETRY_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { double x, y; } Mc2Point2;
typedef enum { MC2_ENTITY_LINE = 1, MC2_ENTITY_CIRCLE = 2, MC2_ENTITY_ARC = 3 } Mc2EntityKind;
typedef struct {
    uint64_t handle;
    uint32_t layer_id;
    uint32_t color_rgb;
    Mc2EntityKind kind;
    union {
        struct { Mc2Point2 start, end; } line;
        struct { Mc2Point2 center; double radius; } circle;
        struct { Mc2Point2 center; double radius, start_radians, end_radians; } arc;
    } geometry;
} Mc2Entity2;
typedef struct { double min_x, min_y, max_x, max_y; } Mc2Bounds2;
int mc2_entity_validate(const Mc2Entity2 *entity);
int mc2_entity_bounds(const Mc2Entity2 *entity, Mc2Bounds2 *out);
#ifdef __cplusplus
}
#endif
#endif
