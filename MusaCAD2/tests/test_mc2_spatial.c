#include "mc2_spatial.h"
#include <assert.h>
#include <math.h>
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
    Mc2Entity2 circle={0};
    circle.kind=MC2_ENTITY_CIRCLE;
    circle.geometry.circle.center=(Mc2Point2){0,0};
    circle.geometry.circle.radius=2;
    Mc2Affine2 shear={1,0,1,1,0,0};
    assert(mc2_entity_world_bounds(&circle,&shear,&out));
    assert(fabs(out.max_x-2.8284271247461903)<1e-12);
    assert(fabs(out.min_x+2.8284271247461903)<1e-12);
    assert(out.max_y==2 && out.min_y==-2);
    Mc2Entity2 arc={0};
    arc.kind=MC2_ENTITY_ARC;
    arc.geometry.arc.center=(Mc2Point2){0,0};
    arc.geometry.arc.radius=2;
    arc.geometry.arc.start_radians=0;
    arc.geometry.arc.end_radians=1.5707963267948966;
    assert(mc2_entity_world_bounds(&arc,&shear,&out));
    assert(fabs(out.max_x-2.8284271247461903)<1e-12);
    assert(out.min_x>=-1e-12 && out.min_y>=-1e-12);
    assert(fabs(out.max_y-2)<1e-12);
    /* Wrapped 270-to-90 degree arc crosses zero: +X cardinal is inside. */
    const double pi=3.14159265358979323846;
    arc.geometry.arc.start_radians=1.5*pi;
    arc.geometry.arc.end_radians=0.5*pi;
    Mc2Affine2 identity=mc2_transform_identity();
    assert(mc2_entity_world_bounds(&arc,&identity,&out));
    assert(fabs(out.max_x-2.0)<1e-12);
    assert(fabs(out.min_x)<1e-12);
    assert(fabs(out.min_y+2.0)<1e-12 && fabs(out.max_y-2.0)<1e-12);
    /* Under shear, the 45-degree stationary point must be included. */
    assert(mc2_entity_world_bounds(&arc,&shear,&out));
    assert(fabs(out.max_x-2.8284271247461903)<1e-12);
    assert(fabs(out.min_x+2.0)<1e-12);
    assert(fabs(out.min_y+2.0)<1e-12 && fabs(out.max_y-2.0)<1e-12);
    /* A nonwrapped left-hand arc must exclude the right cardinal point. */
    arc.geometry.arc.start_radians=0.5*pi;
    arc.geometry.arc.end_radians=1.5*pi;
    assert(mc2_entity_world_bounds(&arc,&identity,&out));
    assert(out.max_x<1e-12 && fabs(out.min_x+2.0)<1e-12);
    a.min_x=3;
    assert(!mc2_transform_bounds(&a,&t,&out));
    return 0;
}
