#include "mc2_geometry.h"
#include <math.h>
#include <stddef.h>
static int valid_point(Mc2Point2 p) { return isfinite(p.x) && isfinite(p.y); }
static int valid_radius(double radius) { return isfinite(radius) && radius > 0.0; }
int mc2_entity_validate(const Mc2Entity2 *e) {
    if (!e || e->color_rgb > 0xffffffu) return 0;
    switch (e->kind) {
    case MC2_ENTITY_LINE:
        return valid_point(e->geometry.line.start) && valid_point(e->geometry.line.end);
    case MC2_ENTITY_CIRCLE:
        return valid_point(e->geometry.circle.center) && valid_radius(e->geometry.circle.radius);
    case MC2_ENTITY_ARC:
        return valid_point(e->geometry.arc.center) && valid_radius(e->geometry.arc.radius) &&
               isfinite(e->geometry.arc.start_radians) && isfinite(e->geometry.arc.end_radians) &&
               e->geometry.arc.start_radians >= 0.0 &&
               e->geometry.arc.end_radians <= 6.28318530717958647693 &&
               e->geometry.arc.start_radians < e->geometry.arc.end_radians;
    default: return 0;
    }
}
static void add_point(Mc2Bounds2 *b, double x, double y) {
    if (x < b->min_x) b->min_x = x;
    if (x > b->max_x) b->max_x = x;
    if (y < b->min_y) b->min_y = y;
    if (y > b->max_y) b->max_y = y;
}
int mc2_entity_bounds(const Mc2Entity2 *e, Mc2Bounds2 *out) {
    if (!out || !mc2_entity_validate(e)) return 0;
    Mc2Bounds2 b;
    if (e->kind == MC2_ENTITY_LINE) {
        Mc2Point2 a = e->geometry.line.start, z = e->geometry.line.end;
        b = (Mc2Bounds2){a.x,a.y,a.x,a.y};
        add_point(&b,z.x,z.y);
    } else {
        Mc2Point2 c = e->kind == MC2_ENTITY_CIRCLE ?
            e->geometry.circle.center : e->geometry.arc.center;
        double r = e->kind == MC2_ENTITY_CIRCLE ?
            e->geometry.circle.radius : e->geometry.arc.radius;
        if (!isfinite(c.x-r) || !isfinite(c.x+r) ||
            !isfinite(c.y-r) || !isfinite(c.y+r)) return 0;
        b = (Mc2Bounds2){c.x-r,c.y-r,c.x+r,c.y+r};
        /* A conservative enclosing box is valid for partial arcs too. */
    }
    *out = b;
    return 1;
}
