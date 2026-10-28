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

int mc2_polyline_signed_area(const Mc2Point2 *vertices,size_t count,
                             int closed,double *out_area) {
    if (!vertices || !out_area || !closed || count<3) return 0;
    /* Translate to the first vertex to reduce cancellation for large offsets. */
    const double ox=vertices[0].x,oy=vertices[0].y;
    if (!isfinite(ox) || !isfinite(oy)) return 0;
    double sum=0.0,compensation=0.0;
    for (size_t i=1;i<count;++i) {
        if (!isfinite(vertices[i].x) || !isfinite(vertices[i].y)) return 0;
    }
    for (size_t i=1;i+1<count;++i) {
        double ax=vertices[i].x-ox,ay=vertices[i].y-oy;
        double bx=vertices[i+1].x-ox,by=vertices[i+1].y-oy;
        double term=ax*by-ay*bx;
        if (!isfinite(term)) return 0;
        double corrected=term-compensation;
        double next=sum+corrected;
        compensation=(next-sum)-corrected;
        sum=next;
        if (!isfinite(sum)) return 0;
    }
    double area=sum*0.5;
    if (!isfinite(area)) return 0;
    *out_area=area;
    return 1;
}

int mc2_polyline_bulge_length(const Mc2Point2 *vertices,
                              const double *bulges, size_t count,
                              int closed, double *out_length) {
    if (!vertices || !count || !out_length) return 0;
    for (size_t i=0;i<count;++i) {
        if (!isfinite(vertices[i].x) || !isfinite(vertices[i].y)) return 0;
    }
    const size_t edges=(closed && count>1) ? count : count-1;
    double total=0.0;
    for (size_t i=0;i<edges;++i) {
        size_t next=(i+1==count) ? 0 : i+1;
        double dx=vertices[next].x-vertices[i].x;
        double dy=vertices[next].y-vertices[i].y;
        double chord=hypot(dx,dy);
        double bulge=bulges ? bulges[i] : 0.0;
        if (!isfinite(chord) || !isfinite(bulge)) return 0;
        double segment=chord;
        if (bulge!=0.0) {
            if (chord==0.0) return 0; /* no defined curved segment */
            double ab=fabs(bulge);
            double factor=ab<=1.0 ?
                (1.0+ab*ab)*(atan(ab)/ab) :
                (ab+1.0/ab)*atan(ab);
            segment=chord*factor;
        }
        if (!isfinite(segment) || !isfinite(total+segment)) return 0;
        total+=segment;
    }
    *out_length=total;
    return 1;
}
