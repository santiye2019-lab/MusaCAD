/* SPDX-License-Identifier: MIT
 * Independent C11 planar instance transforms; no external CAD code.
 */
#include "musa_dwg1032_affine2d.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int valid(const MusaDwg1032Affine2 *t) {
    return isfinite(t->xx) && isfinite(t->xy) && isfinite(t->tx) &&
           isfinite(t->yx) && isfinite(t->yy) && isfinite(t->ty);
}
static int mapped(const MusaDwg1032Affine2 *t,
                  MusaDwg1032Point2 p, MusaDwg1032Point2 *result) {
    if (!isfinite(p.x) || !isfinite(p.y)) return 0;
    const double x=t->xx*p.x+t->xy*p.y+t->tx;
    const double y=t->yx*p.x+t->yy*p.y+t->ty;
    if (!isfinite(x) || !isfinite(y)) return 0;
    result->x=x; result->y=y;
    return 1;
}
int musa_dwg1032_affine2_insert(const MusaDwg1032Point2 *insertion,
    double sx, double sy, double radians, MusaDwg1032Affine2 *out) {
    if (!insertion || !out || overlaps(insertion,sizeof(*insertion),out,sizeof(*out)) ||
        !isfinite(insertion->x) || !isfinite(insertion->y) ||
        !isfinite(sx) || !isfinite(sy) || !isfinite(radians) ||
        sx==0.0 || sy==0.0) return 0;
    const double c=cos(radians),s=sin(radians);
    const MusaDwg1032Affine2 trial={c*sx,-s*sy,insertion->x,
                                    s*sx,c*sy,insertion->y};
    if (!valid(&trial)) return 0;
    *out=trial;
    return 1;
}
int musa_dwg1032_affine2_compose(const MusaDwg1032Affine2 *outer,
    const MusaDwg1032Affine2 *inner, MusaDwg1032Affine2 *out) {
    if (!outer || !inner || !out || !valid(outer) || !valid(inner) ||
        overlaps(outer,sizeof(*outer),out,sizeof(*out)) ||
        overlaps(inner,sizeof(*inner),out,sizeof(*out))) return 0;
    const MusaDwg1032Affine2 t={
        outer->xx*inner->xx+outer->xy*inner->yx,
        outer->xx*inner->xy+outer->xy*inner->yy,
        outer->xx*inner->tx+outer->xy*inner->ty+outer->tx,
        outer->yx*inner->xx+outer->yy*inner->yx,
        outer->yx*inner->xy+outer->yy*inner->yy,
        outer->yx*inner->tx+outer->yy*inner->ty+outer->ty};
    if (!valid(&t)) return 0;
    *out=t;
    return 1;
}
int musa_dwg1032_affine2_apply(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Point2 *points, size_t count, MusaDwg1032Point2 *out) {
    if (!transform || !points || !out || !count ||
        count>SIZE_MAX/sizeof(*points) || !valid(transform)) return 0;
    const size_t bytes=count*sizeof(*points);
    if (overlaps(points,bytes,out,bytes) ||
        overlaps(transform,sizeof(*transform),out,bytes)) return 0;
    for (size_t i=0;i<count;++i) {
        MusaDwg1032Point2 trial;
        if (!mapped(transform,points[i],&trial)) return 0;
    }
    for (size_t i=0;i<count;++i) {
        MusaDwg1032Point2 trial;
        (void)mapped(transform,points[i],&trial);
        out[i]=trial;
    }
    return 1;
}
