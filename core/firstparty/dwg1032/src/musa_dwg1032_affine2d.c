/* SPDX-License-Identifier: MIT
 * Independent C11 planar instance transforms; no external CAD code.
 */
#include "musa_dwg1032_affine2d.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int valid(const MusaDwg1032Affine2 *t) {
    return isfinite(t->xx) && isfinite(t->xy) && isfinite(t->tx) &&
           isfinite(t->yx) && isfinite(t->yy) && isfinite(t->ty);
}
/* A finite affine result can be lost when the two products are added
 * before a large translation of the opposite sign. Keep the ordinary
 * fast path, then retry with fused multiply-add in both orders. */
static double translated_pair(double a, double x, double b, double y,
                              double offset) {
    double result=a*x+b*y+offset;
    if (!isfinite(result)) {
        result=fma(a,x,fma(b,y,offset));
        if (!isfinite(result))
            result=fma(b,y,fma(a,x,offset));
    }
    return result;
}
static int mapped(const MusaDwg1032Affine2 *t,
                  MusaDwg1032Point2 p, MusaDwg1032Point2 *result) {
    if (!isfinite(p.x) || !isfinite(p.y)) return 0;
    const double x=translated_pair(t->xx,p.x,t->xy,p.y,t->tx);
    const double y=translated_pair(t->yx,p.x,t->yy,p.y,t->ty);
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
        translated_pair(outer->xx,inner->xx,outer->xy,inner->yx,0.0),
        translated_pair(outer->xx,inner->xy,outer->xy,inner->yy,0.0),
        translated_pair(outer->xx,inner->tx,outer->xy,inner->ty,outer->tx),
        translated_pair(outer->yx,inner->xx,outer->yy,inner->yx,0.0),
        translated_pair(outer->yx,inner->xy,outer->yy,inner->yy,0.0),
        translated_pair(outer->yx,inner->tx,outer->yy,inner->ty,outer->ty)};
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

int musa_dwg1032_affine2_apply_scratch(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Point2 *points, size_t count, MusaDwg1032Point2 *out,
    MusaDwg1032Point2 *scratch, size_t scratch_capacity) {
    if (!transform || !points || !out || !scratch || !count ||
        count>SIZE_MAX/sizeof(*points) ||
        scratch_capacity<count ||
        scratch_capacity>SIZE_MAX/sizeof(*scratch) ||
        !valid(transform)) return 0;
    const size_t bytes=count*sizeof(*points);
    const size_t scratch_bytes=scratch_capacity*sizeof(*scratch);
    if (overlaps(points,bytes,out,bytes) ||
        overlaps(transform,sizeof(*transform),out,bytes) ||
        overlaps(points,bytes,scratch,scratch_bytes) ||
        overlaps(transform,sizeof(*transform),scratch,scratch_bytes) ||
        overlaps(out,bytes,scratch,scratch_bytes)) return 0;
    for (size_t i=0;i<count;++i)
        if (!mapped(transform,points[i],&scratch[i])) return 0;
    memcpy(out,scratch,bytes);
    return 1;
}

/* Invert a caller-verified planar transform for reverse INSERT mapping.
 * Reject singular or unrepresentable inverses without touching output.
 * Extreme ill-conditioned matrices may be conservatively rejected. */
int musa_dwg1032_affine2_inverse(const MusaDwg1032Affine2 *forward,
                                MusaDwg1032Affine2 *inverse) {
    if (!forward || !inverse || !valid(forward) ||
        overlaps(forward,sizeof(*forward),inverse,sizeof(*inverse)))
        return 0;
    const double determinant=fma(forward->xx,forward->yy,
                                -forward->xy*forward->yx);
    if (!isfinite(determinant) || determinant==0.0) return 0;
    const double xx=forward->yy/determinant;
    const double xy=-forward->xy/determinant;
    const double yx=-forward->yx/determinant;
    const double yy=forward->xx/determinant;
    const MusaDwg1032Affine2 trial={
        xx,xy,translated_pair(-xx,forward->tx,-xy,forward->ty,0.0),
        yx,yy,translated_pair(-yx,forward->tx,-yy,forward->ty,0.0)
    };
    if (!valid(&trial)) return 0;
    *inverse=trial;
    return 1;
}
