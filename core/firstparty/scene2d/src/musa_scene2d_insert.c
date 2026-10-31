/* SPDX-License-Identifier: MIT
 * Independently authored planar block INSERT transform (2026-10-29).
 */
#include "musa_scene2d_insert.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int overlap(const void *a, size_t na, const void *b, size_t nb) {
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < na : x - y < nb;
}
int musa_scene2d_insert_affine(const MusaScene2dInsertPlacement *placement,
                               double out[6]) {
    if (!placement || !out ||
        overlap(placement, sizeof(*placement), out, 6u*sizeof(*out))) return 0;
    const double fields[] = {
        placement->insertion_x, placement->insertion_y,
        placement->base_x, placement->base_y,
        placement->scale_x, placement->scale_y,
        placement->rotation_rad
    };
    for (size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i)
        if (!isfinite(fields[i])) return 0;
    const double c = cos(placement->rotation_rad);
    const double s = sin(placement->rotation_rad);
    const double a = c * placement->scale_x;
    const double b = s * placement->scale_x;
    const double cc = -s * placement->scale_y;
    const double d = c * placement->scale_y;
    /* A finite nonzero scale may underflow during rotation. Preserve
     * singular transforms only when the caller explicitly supplied zero. */
    if ((placement->scale_x != 0.0 && a == 0.0 && b == 0.0) ||
        (placement->scale_y != 0.0 && cc == 0.0 && d == 0.0)) return 0;
    /* Fused products avoid a spurious infinity when a large scaled base
     * cancels a large insertion coordinate to a finite translation. This
     * also rounds each multiply-add only once on very large drawings. */
    double tx = fma(-a, placement->base_x,
                    fma(-cc, placement->base_y, placement->insertion_x));
    double ty = fma(-b, placement->base_x,
                    fma(-d, placement->base_y, placement->insertion_y));
    /* The inner FMA can itself overflow before the outer, opposite-signed
     * product cancels it. Retry with the other product evaluated last.
     * The common finite path remains unchanged. */
    if (!isfinite(tx))
        tx = fma(-cc, placement->base_y,
                 fma(-a, placement->base_x, placement->insertion_x));
    if (!isfinite(ty))
        ty = fma(-d, placement->base_y,
                 fma(-b, placement->base_x, placement->insertion_y));
    const double candidate[6] = {a,b,cc,d,tx,ty};
    for (size_t i = 0; i < 6; ++i)
        if (!isfinite(candidate[i])) return 0;
    for (size_t i = 0; i < 6; ++i) out[i] = candidate[i];
    return 1;
}

/* Transactional conversion of unrelated, already-decoded INSERT placements.
 * No allocation; a malformed late placement cannot partially publish a
 * large drawing's transform list. Inputs remain immutable across passes. */
int musa_scene2d_insert_affine_batch(
    const MusaScene2dInsertPlacement *placements, size_t count,
    double (*out)[6], size_t capacity) {
    if (count>capacity ||
        count>SIZE_MAX/sizeof(*placements) ||
        count>SIZE_MAX/sizeof(*out) ||
        (count && (!placements || !out)) ||
        (count && overlap(placements,count*sizeof(*placements),
                          out,count*sizeof(*out)))) return 0;
    double candidate[6];
    for (size_t i=0;i<count;++i)
        if (!musa_scene2d_insert_affine(&placements[i],candidate)) return 0;
    for (size_t i=0;i<count;++i) {
        (void)musa_scene2d_insert_affine(&placements[i],candidate);
        for (size_t j=0;j<6u;++j) out[i][j]=candidate[j];
    }
    return 1;
}

/* A bounded, resumable page of arbitrary INSERT placements. Unlike a
 * rectangular MINSERT array, each entry has its own decoded placement. */
int musa_scene2d_insert_affine_page(
    const MusaScene2dInsertPlacement *placements, size_t count,
    size_t start, double (*out)[6], size_t capacity,
    size_t *written, size_t *total) {
    if (!written || !total ||
        count>SIZE_MAX/sizeof(*placements) ||
        (count && !placements) ||
        overlap(written,sizeof(*written),total,sizeof(*total)) ||
        (count && (overlap(placements,count*sizeof(*placements),
                           written,sizeof(*written)) ||
                   overlap(placements,count*sizeof(*placements),
                           total,sizeof(*total))))) return 0;
    const size_t remaining=start<count ? count-start : 0u;
    const size_t page_count=remaining<capacity ? remaining : capacity;
    if (page_count>SIZE_MAX/sizeof(*out) ||
        (page_count && !out)) return 0;
    if (page_count &&
        (overlap(placements,count*sizeof(*placements),
                 out,page_count*sizeof(*out)) ||
         overlap(out,page_count*sizeof(*out),written,sizeof(*written)) ||
         overlap(out,page_count*sizeof(*out),total,sizeof(*total))))
        return 0;
    double candidate[6];
    for (size_t i=0;i<page_count;++i)
        if (!musa_scene2d_insert_affine(&placements[start+i],candidate))
            return 0;
    for (size_t i=0;i<page_count;++i) {
        (void)musa_scene2d_insert_affine(&placements[start+i],candidate);
        for (size_t j=0;j<6u;++j) out[i][j]=candidate[j];
    }
    *written=page_count;
    *total=count;
    return 1;
}
