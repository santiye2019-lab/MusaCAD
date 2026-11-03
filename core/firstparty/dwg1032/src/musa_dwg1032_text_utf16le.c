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

/* Exact allocation preflight, independent of any on-disk text framing.
 * The output length is published only after all scalars validate. */
int musa_dwg1032_text_utf16le_measure(
    const uint8_t *utf16le, size_t byte_count, size_t *required) {
    if (!required || (byte_count && !utf16le) || (byte_count&1u) ||
        byte_count>PTRDIFF_MAX ||
        overlaps(utf16le,byte_count,required,sizeof(*required)))
        return 0;
    size_t length=0u, at=0u;
    while (at<byte_count) {
        uint32_t cp=0u;
        if (!scalar(utf16le,byte_count,&at,&cp)) return 0;
        const size_t width=utf8_width(cp);
        if (width>PTRDIFF_MAX-1u-length) return 0;
        length+=width;
    }
    *required=length+1u;
    return 1;
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

/* Measure a bounded record field without consuming the cursor. The
 * temporary cursor validates the complete source span and alignment. */
int musa_dwg1032_text_utf16le_peek_measure(
    const MusaDwg1032Cursor *record, size_t byte_count, size_t *required) {
    if (!record || !required ||
        overlaps(record,sizeof(*record),required,sizeof(*required)) ||
        overlaps(record->bytes,record->byte_count,required,sizeof(*required)))
        return 0;
    MusaDwg1032Cursor trial=*record;
    MusaDwg1032Cursor field;
    if (!musa_dwg1032_cursor_take_bytes(&trial,byte_count,&field))
        return 0;
    return musa_dwg1032_text_utf16le_measure(
        field.bytes,field.byte_count,required);
}

/* The text length is supplied by an independently validated record schema.
 * No on-disk DWG text layout or encoding choice is inferred here. */
int musa_dwg1032_text_utf16le_take(
    MusaDwg1032Cursor *record, size_t byte_count,
    char *utf8, size_t capacity, size_t *written) {
    if (!record || !utf8 || !written ||
        overlaps(record,sizeof(*record),record->bytes,record->byte_count) ||
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


/* Terminator discovery is bounded to a verified cursor frame. The scan is
 * strictly over aligned UTF-16 code units and cannot cross max_field_bytes. */
int musa_dwg1032_text_utf16le_peek_cstr(
    const MusaDwg1032Cursor *record, size_t max_field_bytes,
    size_t *required, size_t *consumed) {
    if (!record || !required || !consumed ||
        overlaps(record,sizeof(*record),required,sizeof(*required)) ||
        overlaps(record,sizeof(*record),consumed,sizeof(*consumed)) ||
        overlaps(record->bytes,record->byte_count,required,sizeof(*required)) ||
        overlaps(record->bytes,record->byte_count,consumed,sizeof(*consumed)) ||
        overlaps(required,sizeof(*required),consumed,sizeof(*consumed)))
        return 0;
    MusaDwg1032Cursor trial=*record, empty;
    if (!musa_dwg1032_cursor_take_bytes(&trial,0u,&empty)) return 0;
    const size_t remaining=record->byte_count-record->bit_position/8u;
    const size_t bound=max_field_bytes<remaining?max_field_bytes:remaining;
    const uint8_t *start=record->bytes
        ? record->bytes+record->bit_position/8u : NULL;
    for (size_t unit=0u; unit<bound/2u; ++unit) {
        const size_t offset=unit*2u;
        if (start[offset]==0u && start[offset+1u]==0u) {
            size_t needed=0u;
            if (!musa_dwg1032_text_utf16le_measure(
                    start,offset,&needed)) return 0;
            *required=needed;
            *consumed=offset+2u;
            return 1;
        }
    }
    return 0;
}

int musa_dwg1032_text_utf16le_take_cstr(
    MusaDwg1032Cursor *record, size_t max_field_bytes,
    char *utf8, size_t capacity, size_t *written) {
    if (!record || !utf8 || !written || !capacity ||
        capacity>PTRDIFF_MAX ||
        overlaps(record,sizeof(*record),record->bytes,record->byte_count) ||
        overlaps(record,sizeof(*record),utf8,capacity) ||
        overlaps(record,sizeof(*record),written,sizeof(*written)) ||
        overlaps(record->bytes,record->byte_count,utf8,capacity) ||
        overlaps(record->bytes,record->byte_count,written,sizeof(*written)) ||
        overlaps(utf8,capacity,written,sizeof(*written)))
        return 0;
    size_t required=0u, consumed=0u;
    if (!musa_dwg1032_text_utf16le_peek_cstr(
            record,max_field_bytes,&required,&consumed) ||
        required>capacity)
        return 0;
    MusaDwg1032Cursor trial=*record, field;
    if (!musa_dwg1032_cursor_take_bytes(&trial,consumed,&field) ||
        !musa_dwg1032_text_utf16le_to_utf8(
            field.bytes,consumed-2u,utf8,capacity,written))
        return 0;
    *record=trial;
    return 1;
}
