#include "mc2_spatial.h"
#include <assert.h>
int main(void) {
    Mc2Bounds2 a={0,0,2,2},b={2,2,4,4},c={3,3,5,5},out;
    assert(mc2_bounds_intersects(&a,&b));
    assert(!mc2_bounds_intersects(&a,&c));
    Mc2Affine2 t={0,1,-1,0,10,20};
    assert(mc2_transform_bounds(&a,&t,&out));
    assert(out.min_x==8 && out.max_x==10 && out.min_y==20 && out.max_y==22);
    Mc2Entity2 line={0};
    line.kind=MC2_ENTITY_LINE;
    line.geometry.line.start=(Mc2Point2){0,0};
    line.geometry.line.end=(Mc2Point2){2,2};
    assert(mc2_entity_world_bounds(&line,&t,&out));
    assert(out.min_x==8 && out.max_y==22);
    a.min_x=3;
    assert(!mc2_transform_bounds(&a,&t,&out));
    return 0;
}
