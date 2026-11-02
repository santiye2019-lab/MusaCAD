/* SPDX-License-Identifier: MIT
 * First-party 2D INSERT-instance transforms for caller-verified geometry.
 */
#ifndef MUSA_DWG1032_AFFINE2D_H
#define MUSA_DWG1032_AFFINE2D_H
#include "musa_dwg1032_point.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Row-major affine map: x'=xx*x+xy*y+tx; y'=yx*x+yy*y+ty.
 * No DWG bytes are decoded; the caller must verify INSERT parameters,
 * units, extrusion and OCS/UCS before using this planar map. */
typedef struct MusaDwg1032Affine2 {
    double xx, xy, tx, yx, yy, ty;
} MusaDwg1032Affine2;
/* Build T(insertion)*R(angle)*S(sx,sy), in radians. Reject zero scales,
 * nonfinite parameters, overflow and aliased output. Output is unchanged
 * on failure. Reflections (negative scale) are supported. */
int musa_dwg1032_affine2_insert(const MusaDwg1032Point2 *insertion,
    double sx, double sy, double radians, MusaDwg1032Affine2 *out);
/* Compose outer(inner(point)) for a particular nested INSERT occurrence.
 * The output must not alias either input; fail atomically. */
int musa_dwg1032_affine2_compose(const MusaDwg1032Affine2 *outer,
    const MusaDwg1032Affine2 *inner, MusaDwg1032Affine2 *out);
/* Reverse a caller-verified planar transform for viewport hit testing and
 * nested INSERT coordinate mapping. Reject singular, nonfinite and
 * unrepresentable inverses. Inputs/output must not overlap; output is
 * unchanged on failure. No DWG parsing or OCS/UCS inference. */
int musa_dwg1032_affine2_inverse(const MusaDwg1032Affine2 *forward,
                                MusaDwg1032Affine2 *inverse);
/* Transform a batch without allocation. Input/output must be disjoint from
 * one another and the transform. Reject nonfinite input or output, overflow,
 * and invalid transforms. Preflight all points before writing ANY result.
 * O(count) time, O(1) memory. */
int musa_dwg1032_affine2_apply(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Point2 *points, size_t count, MusaDwg1032Point2 *out);
/* Caller-scratch variant for large, already-decoded 2D INSERT batches.
 * Map each point once into scratch, then publish the entire batch only
 * after all coordinates are finite. Unlike apply(), avoids a second map
 * pass at the cost of O(count) caller-owned workspace. No allocation.
 * scratch_capacity must be >= count; the entire scratch capacity must be
 * disjoint from points, out and transform. Output must also be disjoint
 * from points and transform. On failure out is completely unchanged;
 * scratch may contain partial results. Does not decode AC1032 pages. */
int musa_dwg1032_affine2_apply_scratch(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Point2 *points, size_t count, MusaDwg1032Point2 *out,
    MusaDwg1032Point2 *scratch, size_t scratch_capacity);
/* Axis-aligned bounds for already-decoded 2D geometry. Degenerate
 * (point/line) bounds are valid. This does not parse DWG extents. */
typedef struct MusaDwg1032Bounds2 {
    double min_x, min_y, max_x, max_y;
} MusaDwg1032Bounds2;

/* Transform all four corners, enclosing rotated/sheared/mirrored INSERTs.
 * Reject inverted/nonfinite bounds, invalid maps, and unrepresentable
 * coordinates without changing output. Inputs/output must not overlap.
 * O(1) time, O(1) memory; no allocation. */
int musa_dwg1032_affine2_bounds(const MusaDwg1032Affine2 *transform,
    const MusaDwg1032Bounds2 *local, MusaDwg1032Bounds2 *world);

/* Inclusive intersection for caller-validated world and viewport bounds:
 * touching edges remain visible, preventing false-negative culling.
 * Reject malformed bounds and aliased output without changing *visible. */
int musa_dwg1032_bounds2_intersects(const MusaDwg1032Bounds2 *world,
    const MusaDwg1032Bounds2 *viewport, unsigned char *visible);
/* Viewport culling for a page of caller-authenticated, already-decoded
 * INSERT occurrences. Every occurrence has its own affine transform and
 * local bounds; effective_visible MUST already include validated owner,
 * layer, viewport and INSERT visibility. This routine does not infer them.
 *
 * Validate the ENTIRE requested page (including hidden entries) before
 * publishing any mask byte. Reject nonfinite or unrepresentable transformed
 * bounds, malformed viewport, invalid flags, overflow and all output aliasing
 * with complete source arrays (including future pages). Edge contact counts
 * as visible. No allocation; O(page_count) time and O(1) workspace.
 * Neither this nor any other post-decode helper opens AC1032 DWG bytes. */
int musa_dwg1032_affine2_cull_page(
    const MusaDwg1032Affine2 *transforms,
    const MusaDwg1032Bounds2 *local_bounds,
    const uint8_t *effective_visible, size_t total_count,
    size_t first, size_t page_count,
    const MusaDwg1032Bounds2 *viewport, uint8_t *page_visible);
#ifdef __cplusplus
}
#endif
#endif
