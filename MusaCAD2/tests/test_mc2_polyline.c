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
    const double pi=3.14159265358979323846;
    Mc2Point2 curved[]={{0,0},{2,0},{2,2}};
    double bulges[]={1.0,0.0,0.0};
    double length=-99;
    assert(mc2_polyline_bulge_length(curved,bulges,3,0,&length));
    assert(fabs(length-(pi+2.0))<1e-12);
    assert(mc2_polyline_bulge_length(curved,bulges,3,1,&length));
    assert(fabs(length-(pi+2.0+sqrt(8.0)))<1e-12);
    bulges[0]=-1.0;
    assert(mc2_polyline_bulge_length(curved,bulges,3,0,&length));
    assert(fabs(length-(pi+2.0))<1e-12);
    bulges[0]=1e-12;
    assert(mc2_polyline_bulge_length(curved,bulges,3,0,&length));
    assert(fabs(length-4.0)<1e-12);
    bulges[0]=NAN;
    assert(!mc2_polyline_bulge_length(curved,bulges,3,0,&length));
    assert(fabs(length-4.0)<1e-12);
    assert(mc2_polyline_bulge_length(curved,0,3,0,&length));
    assert(length==4.0);
    assert(!mc2_polyline_bulge_length(curved,0,0,0,&length));
    assert(length==4.0);
    Mc2Point2 coincident[]={{1,1},{1,1}};
    const double impossible[]={1.0,0.0};
    assert(!mc2_polyline_bulge_length(coincident,impossible,2,0,&length));
    assert(length==4.0);
    Mc2Point2 arc_vertices[]={{0,0},{2,0}};
    double arc_bulges[]={1.0,0.0};
    Mc2Affine2 identity=mc2_transform_identity();
    assert(mc2_polyline_bulge_bounds(arc_vertices,arc_bulges,2,0,&identity,&b));
    assert(fabs(b.min_x)<1e-12 && fabs(b.max_x-2.0)<1e-12);
    assert(fabs(b.min_y+1.0)<1e-12 && fabs(b.max_y)<1e-12);
    assert(mc2_polyline_bulge_bounds(arc_vertices,arc_bulges,2,0,&t,&b));
    assert(fabs(b.min_x-10.0)<1e-12 && fabs(b.max_x-11.0)<1e-12);
    assert(fabs(b.min_y-20.0)<1e-12 && fabs(b.max_y-22.0)<1e-12);
    arc_bulges[0]=-1.0;
    assert(mc2_polyline_bulge_bounds(arc_vertices,arc_bulges,2,0,&identity,&b));
    assert(fabs(b.min_y)<1e-12 && fabs(b.max_y-1.0)<1e-12);
    arc_bulges[0]=1e-12;
    assert(mc2_polyline_bulge_bounds(arc_vertices,arc_bulges,2,0,&identity,&b));
    assert(b.min_y<0 && b.max_y>=0);
    arc_bulges[0]=NAN;
    b.min_x=-777;
    assert(!mc2_polyline_bulge_bounds(arc_vertices,arc_bulges,2,0,&identity,&b));
    assert(b.min_x==-777);
    arc_bulges[0]=1.0;
    assert(mc2_polyline_bulge_bounds(arc_vertices,arc_bulges,2,0,&identity,&b));
    Mc2Point2 duplicate[]={{1,1},{1,1}};
    assert(!mc2_polyline_bulge_bounds(duplicate,arc_bulges,2,0,&identity,&b));
    return 0;
}
