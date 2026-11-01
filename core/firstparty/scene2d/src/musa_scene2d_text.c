/* SPDX-License-Identifier: MIT
 * Independently written Unicode scalar validation and glyph-ink AABBs.
 * Standard C11 only; no legacy CAD, DWG reader or font library code.
 */
#include "musa_scene2d_text.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>

static int ranges_overlap(const void *p, size_t pn, const void *q, size_t qn) {
    if (!pn || !qn) return 0;
    const uintptr_t a = (uintptr_t)p, b = (uintptr_t)q;
    return a <= b ? b-a < pn : a-b < qn;
}
int musa_scene2d_utf8_count(const uint8_t *bytes, size_t length,
                            size_t *codepoints) {
    if (!codepoints || (length && !bytes) ||
        length > PTRDIFF_MAX ||
        ranges_overlap(bytes,length,codepoints,sizeof(*codepoints)))
        return 0;
    size_t i=0, count=0;
    while (i < length) {
        const uint8_t first=bytes[i++];
        uint32_t cp=0;
        size_t rest=0;
        if (first >= 1u && first <= 0x7fu) cp=first;
        else if (first >= 0xc2u && first <= 0xdfu) {
            cp=first & 0x1fu; rest=1;
        } else if (first >= 0xe0u && first <= 0xefu) {
            cp=first & 0x0fu; rest=2;
        } else if (first >= 0xf0u && first <= 0xf4u) {
            cp=first & 0x07u; rest=3;
        } else return 0;
        if (rest > length-i) return 0;
        for (size_t j=0;j<rest;++j) {
            const uint8_t b=bytes[i++];
            if ((b & 0xc0u) != 0x80u) return 0;
            cp=(cp<<6u)|(b & 0x3fu);
        }
        if (cp == 0 || (rest == 1 && cp < 0x80u) ||
            (rest == 2 && cp < 0x800u) ||
            (rest == 3 && cp < 0x10000u) ||
            (cp >= 0xd800u && cp <= 0xdfffu) || cp > 0x10ffffu)
            return 0;
        ++count;
    }
    *codepoints=count;
    return 1;
}
int musa_scene2d_text_run_bounds(const MusaScene2dTextGlyph *glyphs,
                                 size_t count, const double text_affine[6],
                                 MusaScene2dBox *out) {
    if (!out || !text_affine || !count || !glyphs ||
        count > SIZE_MAX / sizeof(*glyphs) ||
        ranges_overlap(out,sizeof(*out),glyphs,count*sizeof(*glyphs)) ||
        ranges_overlap(out,sizeof(*out),text_affine,6*sizeof(double)))
        return 0;
    for (size_t k=0;k<6;++k)
        if (!isfinite(text_affine[k])) return 0;
    double pen_x=0, pen_y=0;
    MusaScene2dBox result={INFINITY,INFINITY,-INFINITY,-INFINITY};
    int any=0;
    for (size_t i=0;i<count;++i) {
        const MusaScene2dTextGlyph *g=&glyphs[i];
        if (g->has_ink > 1 || !isfinite(g->advance_x) ||
            !isfinite(g->advance_y)) return 0;
        if (g->has_ink) {
            const MusaScene2dBox b=g->ink;
            if (!isfinite(b.min_x) || !isfinite(b.min_y) ||
                !isfinite(b.max_x) || !isfinite(b.max_y) ||
                b.min_x > b.max_x || b.min_y > b.max_y) return 0;
            for (unsigned j=0;j<4;++j) {
                const double x=pen_x+((j&1u)?b.max_x:b.min_x);
                const double y=pen_y+((j&2u)?b.max_y:b.min_y);
                const double ax=text_affine[0]*x, cy=text_affine[2]*y;
                const double bx=text_affine[1]*x, dy=text_affine[3]*y;
                /* Ordinary coordinates use the simple fast path. When
                 * its intermediate sum overflows, fused evaluation can
                 * preserve a finite translation after cancellation. */
                double wx=ax+cy+text_affine[4];
                double wy=bx+dy+text_affine[5];
                if (!isfinite(wx))
                    wx=fma(text_affine[0],x,
                           fma(text_affine[2],y,text_affine[4]));
                if (!isfinite(wx))
                    wx=fma(text_affine[2],y,
                           fma(text_affine[0],x,text_affine[4]));
                if (!isfinite(wy))
                    wy=fma(text_affine[1],x,
                           fma(text_affine[3],y,text_affine[5]));
                if (!isfinite(wy))
                    wy=fma(text_affine[3],y,
                           fma(text_affine[1],x,text_affine[5]));
                /* Scale before summing absolute terms: two individually
                 * finite products can overflow their absolute sum even
                 * when opposite signs cancel to a finite world point. */
                const double roundoff=32.0*DBL_EPSILON;
                const double ex=roundoff*fabs(ax)+roundoff*fabs(cy)+
                                roundoff*fabs(text_affine[4])+roundoff;
                const double ey=roundoff*fabs(bx)+roundoff*fabs(dy)+
                                roundoff*fabs(text_affine[5])+roundoff;
                if (!isfinite(wx) || !isfinite(wy) ||
                    !isfinite(ex) || !isfinite(ey)) return 0;
                const double lx=nextafter(wx-ex,-INFINITY);
                const double hx=nextafter(wx+ex,INFINITY);
                const double ly=nextafter(wy-ey,-INFINITY);
                const double hy=nextafter(wy+ey,INFINITY);
                if (!isfinite(lx) || !isfinite(hx) ||
                    !isfinite(ly) || !isfinite(hy)) return 0;
                if (lx<result.min_x) result.min_x=lx;
                if (hx>result.max_x) result.max_x=hx;
                if (ly<result.min_y) result.min_y=ly;
                if (hy>result.max_y) result.max_y=hy;
            }
            any=1;
        }
        /* The final glyph's advance is not used to place any ink.
         * Do not reject valid bounds because an unused trailing advance
         * would overflow the pen (common with malformed font metrics). */
        if (i+1u<count) {
            pen_x+=g->advance_x;
            pen_y+=g->advance_y;
            if (!isfinite(pen_x) || !isfinite(pen_y)) return 0;
        }
    }
    if (!any) return 0;
    *out=result;
    return 1;
}
