/* SPDX-License-Identifier: MIT */
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
static int output_safe(const MusaDwg1032Cursor *c, const void *out,
                       size_t size) {
    return c && out &&
           !overlaps(c,sizeof(*c),out,size) &&
           !overlaps(c->bytes,c->byte_count,out,size);
}
/* DWG scalar payload bytes are little-endian even when the field begins at
 * an arbitrary bit position. Read one complete 8-bit group at a time. */
static int read_le_bits(MusaDwg1032Cursor *c, unsigned bytes,
                        uint64_t *value) {
    uint64_t decoded=0;
    for (unsigned i=0;i<bytes;++i) {
        uint32_t part=0;
        if (!musa_dwg1032_cursor_read_bits(c,8u,&part)) return 0;
        decoded|=(uint64_t)part<<(8u*i);
    }
    *value=decoded;
    return 1;
}
int musa_dwg1032_bitcode_short(MusaDwg1032Cursor *cursor, uint16_t *value) {
    if (!output_safe(cursor,value,sizeof(*value))) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint32_t code=0;
    uint64_t decoded=0;
    if (!musa_dwg1032_cursor_read_bits(&trial,2u,&code)) return 0;
    if (code==0u) {
        if (!read_le_bits(&trial,2u,&decoded)) return 0;
    } else if (code==1u) {
        if (!read_le_bits(&trial,1u,&decoded)) return 0;
    } else if (code==3u) decoded=256u;
    *value=(uint16_t)decoded;
    *cursor=trial;
    return 1;
}
int musa_dwg1032_bitcode_long(MusaDwg1032Cursor *cursor, uint32_t *value) {
    if (!output_safe(cursor,value,sizeof(*value))) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint32_t code=0;
    uint64_t decoded=0;
    if (!musa_dwg1032_cursor_read_bits(&trial,2u,&code) || code==3u)
        return 0;
    if (code==0u) {
        if (!read_le_bits(&trial,4u,&decoded)) return 0;
    } else if (code==1u) {
        if (!read_le_bits(&trial,1u,&decoded)) return 0;
    }
    *value=(uint32_t)decoded;
    *cursor=trial;
    return 1;
}
int musa_dwg1032_bitcode_double(MusaDwg1032Cursor *cursor, double *value) {
    if (!output_safe(cursor,value,sizeof(*value)) || sizeof(double)!=8u ||
        DBL_MANT_DIG!=53 || DBL_MAX_EXP!=1024) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint32_t code=0;
    uint64_t raw=0;
    double decoded=0.0;
    if (!musa_dwg1032_cursor_read_bits(&trial,2u,&code) || code==3u)
        return 0;
    if (code==0u) {
        if (!read_le_bits(&trial,8u,&raw)) return 0;
        memcpy(&decoded,&raw,sizeof(decoded));
        if (!isfinite(decoded)) return 0;
    } else if (code==1u) decoded=1.0;
    *value=decoded;
    *cursor=trial;
    return 1;
}
int musa_dwg1032_bitcode_object_type(MusaDwg1032Cursor *cursor,
                                     uint16_t *value) {
    if (!output_safe(cursor,value,sizeof(*value))) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint32_t code=0;
    uint64_t decoded=0;
    if (!musa_dwg1032_cursor_read_bits(&trial,2u,&code) || code==3u)
        return 0;
    if (!read_le_bits(&trial,code==2u?2u:1u,&decoded)) return 0;
    if (code==1u) decoded+=0x1f0u;
    *value=(uint16_t)decoded;
    *cursor=trial;
    return 1;
}
