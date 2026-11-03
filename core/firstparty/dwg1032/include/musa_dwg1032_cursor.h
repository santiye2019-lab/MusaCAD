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
/* Generic unsigned 64-bit MSB-first fields; NOT verified DWG bitcodes. */
int musa_dwg1032_cursor_read_bits64(MusaDwg1032Cursor *cursor,
                                    unsigned bit_count, uint64_t *value);
int musa_dwg1032_cursor_skip_bits(MusaDwg1032Cursor *cursor,
                                  size_t bit_count);
/* Fixed-width little-endian fields require byte alignment. */
int musa_dwg1032_cursor_read_u16le(MusaDwg1032Cursor *cursor,
                                   uint16_t *value);
int musa_dwg1032_cursor_read_u32le(MusaDwg1032Cursor *cursor,
                                   uint32_t *value);
int musa_dwg1032_cursor_read_u64le(MusaDwg1032Cursor *cursor,
                                   uint64_t *value);
/* Byte-aligned raw bytes. Zero-length reads are allowed, including a NULL
 * destination. Nonempty destinations must be disjoint from the cursor and
 * the entire source buffer. No output or cursor changes on failure. These
 * are generic binary primitives, not decoded AC1032 fields. */
int musa_dwg1032_cursor_read_bytes(MusaDwg1032Cursor *cursor,
                                   uint8_t *destination, size_t count);
int musa_dwg1032_cursor_read_u8(MusaDwg1032Cursor *cursor,
                                uint8_t *value);
/* Returns a bounded byte-aligned subspan without changing the parent.
 * start_byte and span_bytes are relative to the buffer, not bit_position. */
int musa_dwg1032_cursor_slice(const MusaDwg1032Cursor *parent,
                              size_t start_byte, size_t span_bytes,
                              MusaDwg1032Cursor *slice);
/* Atomically consumes a byte-aligned bounded frame. On failure, parent and
 * child remain unchanged. The child owns no memory and cannot outlive bytes. */
int musa_dwg1032_cursor_take_bytes(MusaDwg1032Cursor *parent,
                                   size_t span_bytes,
                                   MusaDwg1032Cursor *child);
/* Generic little-endian uint32 length followed by a byte frame. The caller
 * sets a strict allocation/processing limit. Atomic on every failure.
 * This is NOT an asserted AC1032 section or object framing convention. */
int musa_dwg1032_cursor_take_u32le_frame(MusaDwg1032Cursor *parent,
                                          size_t max_frame_bytes,
                                          MusaDwg1032Cursor *child);
#ifdef __cplusplus
}
#endif
#endif
