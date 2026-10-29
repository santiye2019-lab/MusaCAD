/* SPDX-License-Identifier: MIT
 * Strict, independently authored UTF-16LE Unicode scalar decoder.
 * Standard C11 only. No DWG reader, legacy CAD or external code.
 */
#include "musa_scene2d_utf16.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int scalar(const uint8_t *bytes, size_t length,
                  size_t *position, uint32_t *value) {
    size_t i=*position;
    if (length-i<2) return 0;
    uint32_t first=(uint32_t)bytes[i] | ((uint32_t)bytes[i+1]<<8u);
    i+=2;
    if (first==0u) return 0;
    if (first>=0xd800u && first<=0xdbffu) {
        if (length-i<2) return 0;
        const uint32_t second=(uint32_t)bytes[i] |
                              ((uint32_t)bytes[i+1]<<8u);
        if (second<0xdc00u || second>0xdfffu) return 0;
        first=0x10000u+((first-0xd800u)<<10u)+(second-0xdc00u);
        i+=2;
    } else if (first>=0xdc00u && first<=0xdfffu) {
        return 0;
    }
    *position=i;
    *value=first;
    return 1;
}
int musa_scene2d_utf16le_to_utf8(const uint8_t *utf16le,
                                 size_t byte_length,
                                 uint8_t *utf8,
                                 size_t utf8_capacity,
                                 size_t *written) {
    if (!written || (byte_length && !utf16le) || (byte_length & 1u) ||
        byte_length>PTRDIFF_MAX || utf8_capacity>PTRDIFF_MAX ||
        ((utf8==NULL)!=(utf8_capacity==0)) ||
        overlaps(utf16le,byte_length,written,sizeof(*written)) ||
        overlaps(utf16le,byte_length,utf8,utf8_capacity) ||
        overlaps(utf8,utf8_capacity,written,sizeof(*written))) return 0;
    size_t position=0, needed=0;
    while (position<byte_length) {
        uint32_t cp=0;
        if (!scalar(utf16le,byte_length,&position,&cp)) return 0;
        const size_t n=cp<0x80u?1u:cp<0x800u?2u:cp<0x10000u?3u:4u;
        if (needed>SIZE_MAX-n-1u) return 0;
        needed+=n;
    }
    if (!utf8) { *written=needed; return 1; }
    if (utf8_capacity<=needed) return 0;
    position=0;
    size_t out=0;
    while (position<byte_length) {
        uint32_t cp=0;
        /* Already validated in the first pass, with no intervening writes. */
        (void)scalar(utf16le,byte_length,&position,&cp);
        if (cp<0x80u) {
            utf8[out++]=(uint8_t)cp;
        } else if (cp<0x800u) {
            utf8[out++]=(uint8_t)(0xc0u|(cp>>6u));
            utf8[out++]=(uint8_t)(0x80u|(cp&0x3fu));
        } else if (cp<0x10000u) {
            utf8[out++]=(uint8_t)(0xe0u|(cp>>12u));
            utf8[out++]=(uint8_t)(0x80u|((cp>>6u)&0x3fu));
            utf8[out++]=(uint8_t)(0x80u|(cp&0x3fu));
        } else {
            utf8[out++]=(uint8_t)(0xf0u|(cp>>18u));
            utf8[out++]=(uint8_t)(0x80u|((cp>>12u)&0x3fu));
            utf8[out++]=(uint8_t)(0x80u|((cp>>6u)&0x3fu));
            utf8[out++]=(uint8_t)(0x80u|(cp&0x3fu));
        }
    }
    utf8[out]=0;
    *written=out;
    return 1;
}
