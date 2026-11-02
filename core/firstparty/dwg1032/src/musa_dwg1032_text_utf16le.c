/* SPDX-License-Identifier: MIT
 * First-party C11 UTF-16LE scalar validation and UTF-8 encoding.
 * Input must already be a caller-authenticated text payload.
 */
#include "musa_dwg1032_text_utf16le.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

/* Read a Unicode scalar without using unaligned uint16_t loads. */
static int scalar(const uint8_t *bytes, size_t length, size_t *at,
                  uint32_t *codepoint) {
    if (length-*at<2u) return 0;
    const uint16_t first=(uint16_t)((uint16_t)bytes[*at] |
                                    ((uint16_t)bytes[*at+1u]<<8u));
    *at+=2u;
    if (!first) return 0; /* No embedded NUL in a display label. */
    if (first>=0xdc00u && first<=0xdfffu) return 0;
    if (first>=0xd800u && first<=0xdbffu) {
        if (length-*at<2u) return 0;
        const uint16_t second=(uint16_t)((uint16_t)bytes[*at] |
                                         ((uint16_t)bytes[*at+1u]<<8u));
        if (second<0xdc00u || second>0xdfffu) return 0;
        *at+=2u;
        *codepoint=0x10000u+
            (((uint32_t)first-0xd800u)<<10u)+
            ((uint32_t)second-0xdc00u);
    } else {
        *codepoint=first;
    }
    return 1;
}

static size_t utf8_width(uint32_t cp) {
    if (cp<=0x7fu) return 1u;
    if (cp<=0x7ffu) return 2u;
    if (cp<=0xffffu) return 3u;
    return 4u;
}

static void encode(uint32_t cp, uint8_t *out, size_t *at) {
    if (cp<=0x7fu) {
        out[(*at)++]=(uint8_t)cp;
    } else if (cp<=0x7ffu) {
        out[(*at)++]=(uint8_t)(0xc0u | (cp>>6u));
        out[(*at)++]=(uint8_t)(0x80u | (cp&0x3fu));
    } else if (cp<=0xffffu) {
        out[(*at)++]=(uint8_t)(0xe0u | (cp>>12u));
        out[(*at)++]=(uint8_t)(0x80u | ((cp>>6u)&0x3fu));
        out[(*at)++]=(uint8_t)(0x80u | (cp&0x3fu));
    } else {
        out[(*at)++]=(uint8_t)(0xf0u | (cp>>18u));
        out[(*at)++]=(uint8_t)(0x80u | ((cp>>12u)&0x3fu));
        out[(*at)++]=(uint8_t)(0x80u | ((cp>>6u)&0x3fu));
        out[(*at)++]=(uint8_t)(0x80u | (cp&0x3fu));
    }
}

int musa_dwg1032_text_utf16le_to_utf8(
    const uint8_t *utf16le, size_t byte_count,
    char *utf8, size_t capacity, size_t *written) {
    if ((byte_count && !utf16le) || !utf8 || !written ||
        (byte_count&1u) || !capacity ||
        byte_count>PTRDIFF_MAX || capacity>PTRDIFF_MAX ||
        overlaps(utf16le,byte_count,utf8,capacity) ||
        overlaps(utf16le,byte_count,written,sizeof(*written)) ||
        overlaps(utf8,capacity,written,sizeof(*written)))
        return 0;

    /* First pass validates the entire input and checks output bounds. */
    size_t length=0, at=0;
    while (at<byte_count) {
        uint32_t cp=0;
        if (!scalar(utf16le,byte_count,&at,&cp)) return 0;
        const size_t width=utf8_width(cp);
        if (width>capacity-1u-length) return 0;
        length+=width;
    }

    /* Source is caller-owned and immutable for the duration of the call. */
    at=0;
    size_t position=0;
    while (at<byte_count) {
        uint32_t cp=0;
        if (!scalar(utf16le,byte_count,&at,&cp)) return 0;
        encode(cp,(uint8_t *)utf8,&position);
    }
    utf8[position]='\0';
    *written=position;
    return 1;
}

/* The text length is supplied by an independently validated record schema.
 * No on-disk DWG text layout or encoding choice is inferred here. */
int musa_dwg1032_text_utf16le_take(
    MusaDwg1032Cursor *record, size_t byte_count,
    char *utf8, size_t capacity, size_t *written) {
    if (!record || !utf8 || !written ||
        overlaps(record,sizeof(*record),utf8,capacity) ||
        overlaps(record,sizeof(*record),written,sizeof(*written)) ||
        overlaps(record->bytes,record->byte_count,utf8,capacity) ||
        overlaps(record->bytes,record->byte_count,written,sizeof(*written)))
        return 0;
    MusaDwg1032Cursor trial=*record;
    MusaDwg1032Cursor field;
    if (!musa_dwg1032_cursor_take_bytes(&trial,byte_count,&field) ||
        !musa_dwg1032_text_utf16le_to_utf8(
            field.bytes,field.byte_count,utf8,capacity,written))
        return 0;
    *record=trial;
    return 1;
}
