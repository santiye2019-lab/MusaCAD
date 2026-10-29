/* SPDX-License-Identifier: MIT
 * Independently authored bounded binary cursor primitives.
 * This is not an AC1032 object, page-map, or compression decoder.
 */
#ifndef MUSA_DWG1032_CURSOR_H
#define MUSA_DWG1032_CURSOR_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaDwg1032Cursor {
    const uint8_t *bytes;
    size_t byte_count;
    size_t bit_position;
} MusaDwg1032Cursor;

/* Accepts a caller-verified bounded buffer; no allocation. Rejects sizes
 * whose bit length cannot be represented. Initial state is untouched on
 * failure. Byte order of bit fields is MSB-first within each byte; this is
 * an explicit generic convention, NOT an assertion about DWG bitcodes. */
int musa_dwg1032_cursor_init(MusaDwg1032Cursor *cursor,
                             const uint8_t *bytes, size_t byte_count);
/* All read/skip operations preserve cursor and output on failure. */
int musa_dwg1032_cursor_read_bits(MusaDwg1032Cursor *cursor,
                                  unsigned bit_count, uint32_t *value);
int musa_dwg1032_cursor_skip_bits(MusaDwg1032Cursor *cursor,
                                  size_t bit_count);
/* Fixed-width little-endian fields require byte alignment. */
int musa_dwg1032_cursor_read_u16le(MusaDwg1032Cursor *cursor,
                                   uint16_t *value);
int musa_dwg1032_cursor_read_u32le(MusaDwg1032Cursor *cursor,
                                   uint32_t *value);
/* Returns a bounded byte-aligned subspan without changing the parent.
 * start_byte and span_bytes are relative to the buffer, not bit_position. */
int musa_dwg1032_cursor_slice(const MusaDwg1032Cursor *parent,
                              size_t start_byte, size_t span_bytes,
                              MusaDwg1032Cursor *slice);
#ifdef __cplusplus
}
#endif
#endif
