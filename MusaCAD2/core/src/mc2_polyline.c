#include "mc2_polyline.h"
#include <math.h>
#include <stddef.h>
int mc2_polyline_bounds(const Mc2Point2 *vertices,size_t count,int closed,
                        const Mc2Affine2 *transform,Mc2Bounds2 *out) {
    (void)closed;
    if (!vertices || !count || !transform || !out) return 0;
    Mc2Point2 p;
    if (!mc2_transform_point(transform,vertices[0],&p)) return 0;
    Mc2Bounds2 b={p.x,p.y,p.x,p.y};
    for (size_t i=1;i<count;++i) {
        if (!mc2_transform_point(transform,vertices[i],&p)) return 0;
        if (p.x<b.min_x) b.min_x=p.x;
        if (p.x>b.max_x) b.max_x=p.x;
        if (p.y<b.min_y) b.min_y=p.y;
        if (p.y>b.max_y) b.max_y=p.y;
    }
    *out=b;return 1;
}
double mc2_polyline_length(const Mc2Point2 *vertices,size_t count,int closed) {
    if (!vertices || !count) return NAN;
    double length=0;
    for (size_t i=0;i<count;++i)
        if (!isfinite(vertices[i].x) || !isfinite(vertices[i].y)) return NAN;
    for (size_t i=1;i<count;++i) {
        double d=hypot(vertices[i].x-vertices[i-1].x,vertices[i].y-vertices[i-1].y);
        if (!isfinite(d) || !isfinite(length+d)) return NAN;
        length+=d;
    }
    if (closed && count>1) {
        double d=hypot(vertices[count-1].x-vertices[0].x,vertices[count-1].y-vertices[0].y);
        if (!isfinite(d) || !isfinite(length+d)) return NAN;
        length+=d;
    }
    return length;
}
