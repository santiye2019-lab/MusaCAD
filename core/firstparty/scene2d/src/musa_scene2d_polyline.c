/* SPDX-License-Identifier: MIT
 * Independently authored conservative polyline bounds; no CAD codec code.
 */
#include "musa_scene2d_polyline.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}

static int segment_bounds(const MusaScene2dPolylineVertex *p,
                          const MusaScene2dPolylineVertex *q,
                          MusaScene2dBox *out) {
    MusaScene2dBox box;
    if (!musa_scene2d_line_bounds(p->x,p->y,q->x,q->y,&box))
        return 0;
    if (p->bulge == 0.0) {
        *out = box;
        return 1;
    }
    const double dx = q->x - p->x, dy = q->y - p->y;
    const double chord = hypot(dx,dy);
    if (!isfinite(dx) || !isfinite(dy) || !isfinite(chord) ||
        chord == 0.0) return 0;
    const double abs_bulge = fabs(p->bulge);
    if (abs_bulge <= 1e-4) {
        /* For a minor circular arc the maximum normal deviation from
         * its chord is |bulge| * chord / 2. This avoids the ill-conditioned
         * circle center for almost-straight segments. */
        const double scale = fmax(fmax(fabs(p->x),fabs(q->x)),
                                  fmax(fabs(p->y),fabs(q->y)));
        const double margin = abs_bulge * (0.5*chord) +
                              (64.0*DBL_EPSILON)*scale;
        if (!isfinite(margin)) return 0;
        box.min_x -= margin; box.max_x += margin;
        box.min_y -= margin; box.max_y += margin;
        if (!isfinite(box.min_x) || !isfinite(box.max_x) ||
            !isfinite(box.min_y) || !isfinite(box.max_y)) return 0;
    } else {
        /* The signed center offset from the chord midpoint is
         * chord*(1/bulge - bulge)/4 along the left normal. */
        const double offset = (0.25*chord) *
                              (1.0/p->bulge - p->bulge);
        const double cx = 0.5*p->x + 0.5*q->x - (dy/chord)*offset;
        const double cy = 0.5*p->y + 0.5*q->y + (dx/chord)*offset;
        const double radius = hypot(0.5*chord,offset);
        if (!isfinite(offset) || !isfinite(cx) || !isfinite(cy) ||
            !isfinite(radius) || radius <= 0.0) return 0;
        /* A clockwise arc has the same bounds as the reverse CCW arc. */
        const MusaScene2dPolylineVertex *start =
            p->bulge > 0.0 ? p : q;
        const double vx = start->x - cx, vy = start->y - cy;
        if (!isfinite(vx) || !isfinite(vy)) return 0;
        const double angle = atan2(vy,vx);
        const double sweep = 4.0*atan(abs_bulge);
        MusaScene2dBox arc;
        if (!musa_scene2d_arc_bounds(cx,cy,radius,angle,angle+sweep,
                                     &arc)) return 0;
        /* Include the original chord endpoints despite center rounding. */
        box.min_x = fmin(box.min_x,arc.min_x);
        box.min_y = fmin(box.min_y,arc.min_y);
        box.max_x = fmax(box.max_x,arc.max_x);
        box.max_y = fmax(box.max_y,arc.max_y);
    }
    *out = box;
    return 1;
}

int musa_scene2d_polyline_bounds(const MusaScene2dPolylineVertex *vertices,
                                size_t count, int closed, MusaScene2dBox *out) {
    if (!vertices || !out || count < 2 || (closed != 0 && closed != 1) ||
        count > SIZE_MAX/sizeof(*vertices) ||
        overlaps(vertices,count*sizeof(*vertices),out,sizeof(*out)))
        return 0;
    for (size_t i=0;i<count;++i)
        if (!isfinite(vertices[i].x) || !isfinite(vertices[i].y) ||
            !isfinite(vertices[i].bulge)) return 0;
    const size_t segments = closed ? count : count-1;
    MusaScene2dBox bounds = {0,0,0,0};
    for (size_t i=0;i<segments;++i) {
        const size_t next = i+1 == count ? 0 : i+1;
        MusaScene2dBox b;
        if (!segment_bounds(&vertices[i],&vertices[next],&b)) return 0;
        if (i == 0) bounds = b;
        else {
            bounds.min_x = fmin(bounds.min_x,b.min_x);
            bounds.min_y = fmin(bounds.min_y,b.min_y);
            bounds.max_x = fmax(bounds.max_x,b.max_x);
            bounds.max_y = fmax(bounds.max_y,b.max_y);
        }
    }
    *out = bounds;
    return 1;
}
