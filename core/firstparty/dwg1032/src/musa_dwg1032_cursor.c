/* SPDX-License-Identifier: MIT
 * First-party bounded bit and byte cursor; standard C11 only.
 */
#include "musa_dwg1032_cursor.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int valid(const MusaDwg1032Cursor *cursor) {
    return cursor && cursor->byte_count<=PTRDIFF_MAX/8u &&
           (!cursor->byte_count || cursor->bytes) &&
           cursor->bit_position<=cursor->byte_count*8u &&
           !overlaps(cursor,cursor?sizeof(*cursor):0u,
                     cursor?cursor->bytes:NULL,
                     cursor?cursor->byte_count:0u);
}
int musa_dwg1032_cursor_init(MusaDwg1032Cursor *cursor,
                             const uint8_t *bytes, size_t byte_count) {
    if (!cursor || byte_count>PTRDIFF_MAX/8u ||
        (byte_count && !bytes) ||
        overlaps(cursor,sizeof(*cursor),bytes,byte_count)) return 0;
    MusaDwg1032Cursor next={bytes,byte_count,0};
    *cursor=next;
    return 1;
}
int musa_dwg1032_cursor_read_bits(MusaDwg1032Cursor *cursor,
                                  unsigned bit_count, uint32_t *value) {
    if (!valid(cursor) || !value || !bit_count || bit_count>32u ||
        overlaps(cursor,sizeof(*cursor),value,sizeof(*value)) ||
        overlaps(cursor->bytes,cursor->byte_count,value,sizeof(*value)) ||
        (size_t)bit_count>cursor->byte_count*8u-cursor->bit_position) return 0;
    uint32_t decoded=0;
    for (unsigned i=0;i<bit_count;++i) {
        const size_t bit=cursor->bit_position+i;
        decoded=(decoded<<1u)|
                (uint32_t)((cursor->bytes[bit/8u]>>(7u-(bit%8u)))&1u);
    }
    *value=decoded;
    cursor->bit_position+=(size_t)bit_count;
    return 1;
}
int musa_dwg1032_cursor_read_bits64(MusaDwg1032Cursor *cursor,
                                    unsigned bit_count, uint64_t *value) {
    if (!valid(cursor) || !value || !bit_count || bit_count>64u ||
        overlaps(cursor,sizeof(*cursor),value,sizeof(*value)) ||
        overlaps(cursor->bytes,cursor->byte_count,value,sizeof(*value)) ||
        (size_t)bit_count>cursor->byte_count*8u-cursor->bit_position) return 0;
    uint64_t decoded=0;
    for (unsigned i=0;i<bit_count;++i) {
        const size_t bit=cursor->bit_position+i;
        decoded=(decoded<<1u)|
                (uint64_t)((cursor->bytes[bit/8u]>>(7u-(bit%8u)))&1u);
    }
    *value=decoded;
    cursor->bit_position+=(size_t)bit_count;
    return 1;
}
int musa_dwg1032_cursor_skip_bits(MusaDwg1032Cursor *cursor,
                                  size_t bit_count) {
    if (!valid(cursor) || bit_count>cursor->byte_count*8u-cursor->bit_position)
        return 0;
    cursor->bit_position+=bit_count;
    return 1;
}
static int read_le(MusaDwg1032Cursor *cursor, unsigned count,
                   uint32_t *result, const void *out, size_t out_size) {
    if (!valid(cursor) || !result || !out || (cursor->bit_position%8u) ||
        overlaps(cursor,sizeof(*cursor),out,out_size) ||
        overlaps(cursor->bytes,cursor->byte_count,out,out_size) ||
        (size_t)count>cursor->byte_count-cursor->bit_position/8u) return 0;
    const size_t start=cursor->bit_position/8u;
    uint32_t decoded=0;
    for (unsigned i=0;i<count;++i)
        decoded|=((uint32_t)cursor->bytes[start+i])<<(8u*i);
    *result=decoded;
    return 1;
}
int musa_dwg1032_cursor_read_u16le(MusaDwg1032Cursor *cursor,
                                   uint16_t *value) {
    uint32_t decoded=0;
    if (!read_le(cursor,2u,&decoded,value,sizeof(*value))) return 0;
    *value=(uint16_t)decoded;
    cursor->bit_position+=16u;
    return 1;
}
int musa_dwg1032_cursor_read_u32le(MusaDwg1032Cursor *cursor,
                                   uint32_t *value) {
    uint32_t decoded=0;
    if (!read_le(cursor,4u,&decoded,value,sizeof(*value))) return 0;
    *value=decoded;
    cursor->bit_position+=32u;
    return 1;
}
int musa_dwg1032_cursor_read_u64le(MusaDwg1032Cursor *cursor,
                                   uint64_t *value) {
    if (!valid(cursor) || !value || (cursor->bit_position%8u) ||
        overlaps(cursor,sizeof(*cursor),value,sizeof(*value)) ||
        overlaps(cursor->bytes,cursor->byte_count,value,sizeof(*value)) ||
        8u>cursor->byte_count-cursor->bit_position/8u) return 0;
    const size_t start=cursor->bit_position/8u;
    uint64_t decoded=0;
    for (unsigned i=0;i<8u;++i)
        decoded|=((uint64_t)cursor->bytes[start+i])<<(8u*i);
    *value=decoded;
    cursor->bit_position+=64u;
    return 1;
}
int musa_dwg1032_cursor_read_bytes(MusaDwg1032Cursor *cursor,
                                   uint8_t *destination, size_t count) {
    if (!valid(cursor) || (cursor->bit_position%8u) ||
        (count && !destination) ||
        count>cursor->byte_count-cursor->bit_position/8u ||
        overlaps(cursor,sizeof(*cursor),destination,count) ||
        overlaps(cursor->bytes,cursor->byte_count,destination,count)) return 0;
    if (count)
        memcpy(destination,cursor->bytes+cursor->bit_position/8u,count);
    cursor->bit_position+=count*8u;
    return 1;
}
int musa_dwg1032_cursor_read_u8(MusaDwg1032Cursor *cursor,
                                uint8_t *value) {
    return musa_dwg1032_cursor_read_bytes(cursor,value,1u);
}
int musa_dwg1032_cursor_slice(const MusaDwg1032Cursor *parent,
                              size_t start_byte, size_t span_bytes,
                              MusaDwg1032Cursor *slice) {
    if (!valid(parent) || !slice ||
        overlaps(parent,sizeof(*parent),slice,sizeof(*slice)) ||
        overlaps(parent->bytes,parent->byte_count,slice,sizeof(*slice)) ||
        start_byte>parent->byte_count ||
        span_bytes>parent->byte_count-start_byte) return 0;
    /* Pointer arithmetic on NULL is forbidden even for an empty slice. */
    const uint8_t *first=parent->bytes ? parent->bytes+start_byte : NULL;
    return musa_dwg1032_cursor_init(slice,first,span_bytes);
}

int musa_dwg1032_cursor_take_bytes(MusaDwg1032Cursor *parent,
                                   size_t span_bytes,
                                   MusaDwg1032Cursor *child) {
    if (!valid(parent) || !child || (parent->bit_position%8u) ||
        span_bytes>parent->byte_count-parent->bit_position/8u) return 0;
    if (!musa_dwg1032_cursor_slice(parent,parent->bit_position/8u,
                                   span_bytes,child)) return 0;
    parent->bit_position+=span_bytes*8u;
    return 1;
}

int musa_dwg1032_cursor_take_u32le_frame(MusaDwg1032Cursor *parent,
                                          size_t max_frame_bytes,
                                          MusaDwg1032Cursor *child) {
    if (!valid(parent) || !child ||
        overlaps(parent,sizeof(*parent),child,sizeof(*child)) ||
        overlaps(parent->bytes,parent->byte_count,child,sizeof(*child)))
        return 0;
    MusaDwg1032Cursor trial=*parent;
    MusaDwg1032Cursor frame;
    uint32_t length=0;
    if (!musa_dwg1032_cursor_read_u32le(&trial,&length) ||
        (size_t)length>max_frame_bytes ||
        !musa_dwg1032_cursor_take_bytes(&trial,(size_t)length,&frame))
        return 0;
    *child=frame;
    *parent=trial;
    return 1;
}
