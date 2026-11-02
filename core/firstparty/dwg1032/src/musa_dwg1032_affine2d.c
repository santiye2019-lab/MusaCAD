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
/* Fused multiply-add limits intermediate rounding and overflow.
 * Try both accumulation orders because a finite result can depend on
 * which product is combined with the translation first. */
static double translated_pair(double a, double x, double b, double y,
                              double offset) {
    double result=fma(a,x,fma(b,y,offset));
    if (!isfinite(result))
        result=fma(b,y,fma(a,x,offset));
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
        if (!mapped(transform,points[i],&trial)) return 0;
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
    /* Normalize each row independently. Computing the raw determinant
     * first can overflow (large scales) or underflow (small scales) even
     * when all four inverse coefficients are representable. */
    const double row_x=fmax(fabs(forward->xx),fabs(forward->xy));
    const double row_y=fmax(fabs(forward->yx),fabs(forward->yy));
    if (row_x==0.0 || row_y==0.0) return 0;
    const double a=forward->xx/row_x, b=forward->xy/row_x;
    const double c=forward->yx/row_y, d=forward->yy/row_y;
    const double determinant=fma(a,d,-b*c);
    if (!isfinite(determinant) || determinant==0.0) return 0;
    const double xx=(d/determinant)/row_x;
    const double xy=(-b/determinant)/row_y;
    const double yx=(-c/determinant)/row_x;
    const double yy=(a/determinant)/row_y;
    const MusaDwg1032Affine2 trial={
        xx,xy,translated_pair(-xx,forward->tx,-xy,forward->ty,0.0),
        yx,yy,translated_pair(-yx,forward->tx,-yy,forward->ty,0.0)
    };
    if (!valid(&trial)) return 0;
    *inverse=trial;
    return 1;
}

/* Transform the four actual corners. A two-corner shortcut is incorrect for
 * rotated, reflected and sheared INSERTs and can hide visible geometry. */
static int valid_bounds(const MusaDwg1032Bounds2 *b) {
    return b && isfinite(b->min_x) && isfinite(b->min_y) &&
           isfinite(b->max_x) && isfinite(b->max_y) &&
           b->min_x<=b->max_x && b->min_y<=b->max_y;
}

int musa_dwg1032_affine2_bounds(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Bounds2 *local, MusaDwg1032Bounds2 *world) {
    if (!transform || !valid(transform) || !valid_bounds(local) || !world ||
        overlaps(transform,sizeof(*transform),world,sizeof(*world)) ||
        overlaps(local,sizeof(*local),world,sizeof(*world)))
        return 0;
    const MusaDwg1032Point2 corners[4]={
        {local->min_x,local->min_y},
        {local->min_x,local->max_y},
        {local->max_x,local->min_y},
        {local->max_x,local->max_y}
    };
    MusaDwg1032Bounds2 result={0.0,0.0,0.0,0.0};
    for (size_t i=0;i<4u;++i) {
        MusaDwg1032Point2 point;
        if (!mapped(transform,corners[i],&point)) return 0;
        if (!i) {
            result.min_x=result.max_x=point.x;
            result.min_y=result.max_y=point.y;
        } else {
            result.min_x=fmin(result.min_x,point.x);
            result.min_y=fmin(result.min_y,point.y);
            result.max_x=fmax(result.max_x,point.x);
            result.max_y=fmax(result.max_y,point.y);
        }
    }
    *world=result;
    return 1;
}

int musa_dwg1032_bounds2_intersects(const MusaDwg1032Bounds2 *world,
    const MusaDwg1032Bounds2 *viewport, unsigned char *visible) {
    if (!valid_bounds(world) || !valid_bounds(viewport) || !visible ||
        overlaps(world,sizeof(*world),visible,sizeof(*visible)) ||
        overlaps(viewport,sizeof(*viewport),visible,sizeof(*visible)))
        return 0;
    *visible=(unsigned char)(world->max_x>=viewport->min_x &&
        viewport->max_x>=world->min_x &&
        world->max_y>=viewport->min_y &&
        viewport->max_y>=world->min_y);
    return 1;
}

/* Fail-closed viewport culling on already-authenticated per-occurrence
 * bounds and transforms. A complete first pass prevents a late malformed
 * object from publishing an earlier, misleading viewport mask. */
int musa_dwg1032_affine2_cull_page(
    const MusaDwg1032Affine2 *transforms,
    const MusaDwg1032Bounds2 *local_bounds,
    const uint8_t *effective_visible, size_t total_count,
    size_t first, size_t page_count,
    const MusaDwg1032Bounds2 *viewport, uint8_t *page_visible) {
    if (!transforms || !local_bounds || !effective_visible || !viewport ||
        !page_visible || !valid_bounds(viewport) || !page_count ||
        first>total_count || page_count>total_count-first ||
        total_count>SIZE_MAX/sizeof(*transforms) ||
        total_count>SIZE_MAX/sizeof(*local_bounds))
        return 0;
    if (overlaps(page_visible,page_count,transforms,
                 total_count*sizeof(*transforms)) ||
        overlaps(page_visible,page_count,local_bounds,
                 total_count*sizeof(*local_bounds)) ||
        overlaps(page_visible,page_count,effective_visible,total_count) ||
        overlaps(page_visible,page_count,viewport,sizeof(*viewport)))
        return 0;

    for (size_t i=0;i<page_count;++i) {
        const size_t at=first+i;
        MusaDwg1032Bounds2 world;
        if (effective_visible[at]>1u ||
            !musa_dwg1032_affine2_bounds(&transforms[at],
                                        &local_bounds[at],&world))
            return 0;
    }
    for (size_t i=0;i<page_count;++i) {
        const size_t at=first+i;
        MusaDwg1032Bounds2 world;
        /* The preflight guarantees these operations succeed while inputs
         * remain immutable for the duration of the call. */
        (void)musa_dwg1032_affine2_bounds(&transforms[at],
                                         &local_bounds[at],&world);
        page_visible[i]=(uint8_t)(effective_visible[at] &&
            world.max_x>=viewport->min_x &&
            viewport->max_x>=world.min_x &&
            world.max_y>=viewport->min_y &&
            viewport->max_y>=world.min_y);
    }
    return 1;
}

/* The caller owns scratch, so transformed bounds are computed only once.
 * Reject every invalid occurrence (even if hidden) before publishing the
 * complete viewport mask. Source arrays and viewport must stay immutable
 * for the duration of this call. */
int musa_dwg1032_affine2_cull_page_scratch(
    const MusaDwg1032Affine2 *transforms,
    const MusaDwg1032Bounds2 *local_bounds,
    const uint8_t *effective_visible, size_t total_count,
    size_t first, size_t page_count,
    const MusaDwg1032Bounds2 *viewport, uint8_t *page_visible,
    uint8_t *scratch, size_t scratch_capacity) {
    if (!transforms || !local_bounds || !effective_visible || !viewport ||
        !page_visible || !scratch || !valid_bounds(viewport) ||
        !page_count || first>total_count || page_count>total_count-first ||
        scratch_capacity<page_count || scratch_capacity>PTRDIFF_MAX ||
        total_count>SIZE_MAX/sizeof(*transforms) ||
        total_count>SIZE_MAX/sizeof(*local_bounds))
        return 0;
    const size_t transform_bytes=total_count*sizeof(*transforms);
    const size_t bounds_bytes=total_count*sizeof(*local_bounds);
    const void *sources[]={transforms,local_bounds,effective_visible,viewport};
    const size_t source_bytes[]={transform_bytes,bounds_bytes,
                                 total_count,sizeof(*viewport)};
    if (overlaps(page_visible,page_count,scratch,scratch_capacity))
        return 0;
    for (size_t i=0u;i<4u;++i)
        if (overlaps(page_visible,page_count,sources[i],source_bytes[i]) ||
            overlaps(scratch,scratch_capacity,sources[i],source_bytes[i]))
            return 0;
    for (size_t i=0u;i<page_count;++i) {
        const size_t at=first+i;
        MusaDwg1032Bounds2 world;
        if (effective_visible[at]>1u ||
            !musa_dwg1032_affine2_bounds(&transforms[at],
                                        &local_bounds[at],&world))
            return 0;
        scratch[i]=(uint8_t)(effective_visible[at] &&
            world.max_x>=viewport->min_x &&
            viewport->max_x>=world.min_x &&
            world.max_y>=viewport->min_y &&
            viewport->max_y>=world.min_y);
    }
    memcpy(page_visible,scratch,page_count);
    return 1;
}
