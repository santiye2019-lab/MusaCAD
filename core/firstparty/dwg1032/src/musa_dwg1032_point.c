/* SPDX-License-Identifier: MIT
 * Independently authored C11 point bitcodes; no third-party CAD code.
 */
#include "musa_dwg1032_point.h"
#include "musa_dwg1032_bitcode.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int output_safe(const MusaDwg1032Cursor *cursor,
                       const void *output, size_t bytes) {
    return cursor && output &&
           !overlaps(cursor,sizeof(*cursor),output,bytes) &&
           !overlaps(cursor->bytes,cursor->byte_count,output,bytes);
}
static int read_rd(MusaDwg1032Cursor *cursor, double *value) {
    uint64_t raw=0;
    for (unsigned i=0;i<8u;++i) {
        uint32_t octet=0;
        if (!musa_dwg1032_cursor_read_bits(cursor,8u,&octet)) return 0;
        raw|=(uint64_t)octet<<(i*8u);
    }
    double decoded=0.0;
    memcpy(&decoded,&raw,sizeof(decoded));
    if (!isfinite(decoded)) return 0;
    *value=decoded;
    return 1;
}
static int decode(MusaDwg1032Cursor *cursor, void *output,
                  size_t output_bytes, unsigned count, int bitcoded,
                  double values[3]) {
    if (!output_safe(cursor,output,output_bytes) ||
        sizeof(double)!=8u || DBL_MANT_DIG!=53 || DBL_MAX_EXP!=1024)
        return 0;
    MusaDwg1032Cursor trial=*cursor;
    for (unsigned i=0;i<count;++i) {
        if (!(bitcoded ? musa_dwg1032_bitcode_double(&trial,&values[i])
                       : read_rd(&trial,&values[i]))) return 0;
    }
    *cursor=trial;
    return 1;
}
int musa_dwg1032_point2rd(MusaDwg1032Cursor *cursor, MusaDwg1032Point2 *point) {
    double v[3]={0};
    if (!decode(cursor,point,sizeof(*point),2u,0,v)) return 0;
    point->x=v[0]; point->y=v[1];
    return 1;
}
int musa_dwg1032_point3rd(MusaDwg1032Cursor *cursor, MusaDwg1032Point3 *point) {
    double v[3]={0};
    if (!decode(cursor,point,sizeof(*point),3u,0,v)) return 0;
    point->x=v[0]; point->y=v[1]; point->z=v[2];
    return 1;
}
int musa_dwg1032_point2bd(MusaDwg1032Cursor *cursor, MusaDwg1032Point2 *point) {
    double v[3]={0};
    if (!decode(cursor,point,sizeof(*point),2u,1,v)) return 0;
    point->x=v[0]; point->y=v[1];
    return 1;
}
int musa_dwg1032_point3bd(MusaDwg1032Cursor *cursor, MusaDwg1032Point3 *point) {
    double v[3]={0};
    if (!decode(cursor,point,sizeof(*point),3u,1,v)) return 0;
    point->x=v[0]; point->y=v[1]; point->z=v[2];
    return 1;
}
