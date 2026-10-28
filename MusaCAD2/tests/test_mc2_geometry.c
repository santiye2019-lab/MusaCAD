#include "mc2_geometry.h"
#include <assert.h>
#include <math.h>
int main(void) {
    Mc2Entity2 e = {0};
    Mc2Bounds2 b;
    e.kind = MC2_ENTITY_LINE;
    e.color_rgb = 0x00ff00;
    e.geometry.line.start = (Mc2Point2){-2,5};
    e.geometry.line.end = (Mc2Point2){7,-3};
    assert(mc2_entity_validate(&e));
    assert(mc2_entity_bounds(&e,&b));
    assert(b.min_x == -2 && b.min_y == -3 && b.max_x == 7 && b.max_y == 5);
    e.kind = MC2_ENTITY_CIRCLE;
    e.geometry.circle.center = (Mc2Point2){4,5};
    e.geometry.circle.radius = 3;
    assert(mc2_entity_bounds(&e,&b));
    assert(b.min_x == 1 && b.max_x == 7 && b.min_y == 2 && b.max_y == 8);
    e.geometry.circle.radius = -1;
    assert(!mc2_entity_validate(&e));
    e.kind = MC2_ENTITY_ARC;
    e.geometry.arc.center = (Mc2Point2){0,0};
    e.geometry.arc.radius = 2;
    e.geometry.arc.start_radians = 0;
    e.geometry.arc.end_radians = 3.141592653589793;
    assert(mc2_entity_bounds(&e,&b));
    assert(b.min_x == -2 && b.max_y == 2);
    e.geometry.arc.end_radians = NAN;
    assert(!mc2_entity_validate(&e));
    return 0;
}
