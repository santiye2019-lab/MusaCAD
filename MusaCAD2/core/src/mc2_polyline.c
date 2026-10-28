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

static void mc2_bounds_include(Mc2Bounds2 *b, Mc2Point2 p) {
    if (p.x<b->min_x) b->min_x=p.x;
    if (p.x>b->max_x) b->max_x=p.x;
    if (p.y<b->min_y) b->min_y=p.y;
    if (p.y>b->max_y) b->max_y=p.y;
}
static int mc2_angle_on_sweep(double angle,double start,double sweep) {
    const double tau=6.28318530717958647693;
    double delta=fmod(sweep>=0 ? angle-start : start-angle,tau);
    if (delta<0) delta+=tau;
    return delta<=fabs(sweep)+1e-12;
}
int mc2_polyline_bulge_bounds(const Mc2Point2 *vertices,
                              const double *bulges, size_t count, int closed,
                              const Mc2Affine2 *transform, Mc2Bounds2 *out) {
    if (!vertices || !count || !transform || !out) return 0;
    Mc2Bounds2 bounds;
    if (!mc2_polyline_bounds(vertices,count,closed,transform,&bounds)) return 0;
    const size_t edges=(closed && count>1) ? count : count-1;
    for (size_t i=0;i<edges;++i) {
        const double bulge=bulges ? bulges[i] : 0.0;
        if (!isfinite(bulge)) return 0;
        if (bulge==0.0) continue;
        const Mc2Point2 p=vertices[i],q=vertices[(i+1)%count];
        const double dx=q.x-p.x,dy=q.y-p.y;
        const double chord=hypot(dx,dy);
        if (!isfinite(chord) || chord==0.0) return 0;
        if (fabs(bulge)<1e-8) {
            /* The minor arc lies within a sagitta-wide strip around its
               chord; avoid an ill-conditioned remote circle center. */
            Mc2Point2 a,z;
            if (!mc2_transform_point(transform,p,&a) ||
                !mc2_transform_point(transform,q,&z)) return 0;
            const double sagitta=0.5*fabs(bulge)*chord;
            const double nx=-dy/chord,ny=dx/chord;
            const double ex=sagitta*fabs(transform->a*nx+transform->c*ny);
            const double ey=sagitta*fabs(transform->b*nx+transform->d*ny);
            Mc2Point2 low={fmin(a.x,z.x)-ex,fmin(a.y,z.y)-ey};
            Mc2Point2 high={fmax(a.x,z.x)+ex,fmax(a.y,z.y)+ey};
            if (!isfinite(low.x) || !isfinite(low.y) ||
                !isfinite(high.x) || !isfinite(high.y)) return 0;
            mc2_bounds_include(&bounds,low);
            mc2_bounds_include(&bounds,high);
            continue;
        }
        const double center_factor=0.25*(1.0/bulge-bulge);
        const double cx=0.5*p.x+0.5*q.x-center_factor*dy;
        const double cy=0.5*p.y+0.5*q.y+center_factor*dx;
        const double ab=fabs(bulge);
        const double radius=0.25*chord*(ab+1.0/ab);
        const double start=atan2(p.y-cy,p.x-cx);
        const double sweep=4.0*atan(bulge);
        if (!isfinite(cx) || !isfinite(cy) || !isfinite(radius) ||
            radius<=0 || !isfinite(start) || !isfinite(sweep)) return 0;
        const double xangle=atan2(transform->c,transform->a);
        const double yangle=atan2(transform->d,transform->b);
        const double candidates[]={xangle,xangle+3.14159265358979323846,
                                   yangle,yangle+3.14159265358979323846};
        for (size_t k=0;k<4;++k) {
            if (!mc2_angle_on_sweep(candidates[k],start,sweep)) continue;
            Mc2Point2 point={cx+radius*cos(candidates[k]),
                             cy+radius*sin(candidates[k])};
            Mc2Point2 transformed;
            if (!mc2_transform_point(transform,point,&transformed)) return 0;
            mc2_bounds_include(&bounds,transformed);
        }
    }
    *out=bounds;
    return 1;
}
