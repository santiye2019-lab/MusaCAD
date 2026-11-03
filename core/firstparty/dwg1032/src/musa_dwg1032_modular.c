/* SPDX-License-Identifier: MIT
 * Independent implementation from publicly described DWG MC/MS integer
 * conventions. No third-party implementation or code imported.
 */
#include "musa_dwg1032_modular.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int safe(const MusaDwg1032Cursor *cursor, const void *output,
                size_t bytes) {
    return cursor && output &&
           !overlaps(cursor,sizeof(*cursor),output,bytes) &&
           !overlaps(cursor->bytes,cursor->byte_count,output,bytes) &&
           cursor->bit_position%8u==0u;
}
int musa_dwg1032_modular_unsigned(MusaDwg1032Cursor *cursor, uint64_t *value) {
    if (!safe(cursor,value,sizeof(*value))) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint64_t decoded=0;
    for (unsigned i=0;i<10u;++i) {
        uint8_t part=0;
        if (!musa_dwg1032_cursor_read_u8(&trial,&part)) return 0;
        const uint64_t payload=(uint64_t)(part&0x7fu);
        const unsigned shift=i*7u;
        if (payload>(UINT64_MAX>>shift)) return 0;
        decoded|=payload<<shift;
        if (!(part&0x80u)) {
            if (i && !payload) return 0;
            *value=decoded;
            *cursor=trial;
            return 1;
        }
    }
    return 0;
}
int musa_dwg1032_modular_signed(MusaDwg1032Cursor *cursor, int64_t *value) {
    if (!safe(cursor,value,sizeof(*value))) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint64_t magnitude=0;
    uint8_t previous=0;
    for (unsigned i=0;i<10u;++i) {
        uint8_t part=0;
        if (!musa_dwg1032_cursor_read_u8(&trial,&part)) return 0;
        const int more=(part&0x80u)!=0;
        const uint64_t payload=(uint64_t)(part&(more?0x7fu:0x3fu));
        const unsigned shift=i*7u;
        const int negative=!more && (part&0x40u)!=0;
        /* Only the negative terminal group may represent 2^63.
         * Positive 2^63 is out of range; avoid signed negation of
         * INT64_MIN, which would be undefined behaviour in C11. */
        const uint64_t limit=negative
            ? (UINT64_C(1)<<63u) : (uint64_t)INT64_MAX;
        if (payload>(limit>>shift)) return 0;
        magnitude|=payload<<shift;
        /* Earlier groups may already contain low bits, so checking
         * only the terminal group's width is insufficient for -2^63. */
        if (magnitude>limit) return 0;
        if (!more) {
            if ((negative && !magnitude) ||
                (i && !payload && !(previous&0x40u))) return 0;
            *value=negative
                ? (magnitude==(UINT64_C(1)<<63u)
                    ? INT64_MIN : -(int64_t)magnitude)
                : (int64_t)magnitude;
            *cursor=trial;
            return 1;
        }
        previous=(uint8_t)(part&0x7fu);
    }
    return 0;
}
int musa_dwg1032_modular_short(MusaDwg1032Cursor *cursor, uint32_t *value) {
    if (!safe(cursor,value,sizeof(*value))) return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint16_t first=0;
    if (!musa_dwg1032_cursor_read_u16le(&trial,&first)) return 0;
    uint32_t decoded=(uint32_t)(first&0x7fffu);
    if (first&0x8000u) {
        uint16_t second=0;
        if (!musa_dwg1032_cursor_read_u16le(&trial,&second) ||
            (second&0x8000u) || !(second&0x7fffu)) return 0;
        decoded|=(uint32_t)(second&0x7fffu)<<15u;
    }
    *value=decoded;
    *cursor=trial;
    return 1;
}
