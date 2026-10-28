#include "mc2_polyline.h"
#include <assert.h>
#include <math.h>
int main(void) {
    Mc2Point2 p[]={{0,0},{3,0},{3,4}};
    Mc2Bounds2 b;
    Mc2Affine2 t={0,1,-1,0,10,20};
    assert(mc2_polyline_bounds(p,3,0,&t,&b));
    assert(b.min_x==6 && b.max_x==10 && b.min_y==20 && b.max_y==23);
    assert(mc2_polyline_length(p,3,0)==7);
    assert(mc2_polyline_length(p,3,1)==12);
    p[2].x=NAN;
    assert(!mc2_polyline_bounds(p,3,0,&t,&b));
    assert(isnan(mc2_polyline_length(p,3,0)));
    return 0;
}
