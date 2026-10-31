/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_handle.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int valid_shape(uint8_t code, uint8_t length) {
    if (length>8u) return 0;
    if (code>=2u && code<=5u) return 1;
    if (code==6u || code==8u) return length==0u;
    if (code==10u || code==12u) return length!=0u;
    return 0;
}
int musa_dwg1032_handle_read(MusaDwg1032Cursor *cursor,
                             MusaDwg1032Handle *handle) {
    if (!cursor || !handle ||
        overlaps(cursor,sizeof(*cursor),handle,sizeof(*handle)) ||
        overlaps(cursor->bytes,cursor->byte_count,handle,sizeof(*handle)))
        return 0;
    MusaDwg1032Cursor trial=*cursor;
    uint32_t code=0, length=0;
    if (!musa_dwg1032_cursor_read_bits(&trial,4u,&code) ||
        !musa_dwg1032_cursor_read_bits(&trial,4u,&length) ||
        !valid_shape((uint8_t)code,(uint8_t)length)) return 0;
    uint64_t payload=0;
    for (uint32_t i=0;i<length;++i) {
        uint32_t part=0;
        if (!musa_dwg1032_cursor_read_bits(&trial,8u,&part)) return 0;
        payload=(payload<<8u)|(uint64_t)part;
    }
    const MusaDwg1032Handle decoded={payload,(uint8_t)code,(uint8_t)length};
    *handle=decoded;
    *cursor=trial;
    return 1;
}
int musa_dwg1032_handle_resolve(const MusaDwg1032Handle *handle,
                                uint64_t base, uint64_t *resolved) {
    if (!handle || !resolved ||
        overlaps(handle,sizeof(*handle),resolved,sizeof(*resolved)) ||
        !valid_shape(handle->code,handle->length)) return 0;
    uint64_t result=0;
    if (handle->code>=2u && handle->code<=5u) result=handle->payload;
    else if (handle->code==6u) {
        if (base==UINT64_MAX || handle->payload) return 0;
        result=base+1u;
    } else if (handle->code==8u) {
        if (!base || handle->payload) return 0;
        result=base-1u;
    } else if (handle->code==10u) {
        if (handle->payload>UINT64_MAX-base) return 0;
        result=base+handle->payload;
    } else if (handle->code==12u) {
        if (handle->payload>base) return 0;
        result=base-handle->payload;
    } else return 0;
    *resolved=result;
    return 1;
}
