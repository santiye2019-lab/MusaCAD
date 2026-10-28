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
    double area=999;
    assert(mc2_polyline_signed_area(p,3,1,&area) && area==6);
    Mc2Point2 reversed[]={{0,0},{3,4},{3,0}};
    assert(mc2_polyline_signed_area(reversed,3,1,&area) && area==-6);
    assert(!mc2_polyline_signed_area(p,3,0,&area) && area==-6);
    Mc2Point2 offset[]={{1000000000,1000000000},
                        {1000000003,1000000000},
                        {1000000003,1000000004}};
    assert(mc2_polyline_signed_area(offset,3,1,&area) && area==6);
    p[2].x=NAN;
    assert(!mc2_polyline_signed_area(p,3,1,&area) && area==6);
    assert(!mc2_polyline_bounds(p,3,0,&t,&b));
    assert(isnan(mc2_polyline_length(p,3,0)));
    return 0;
}
