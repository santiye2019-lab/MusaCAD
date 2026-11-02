/* SPDX-License-Identifier: MIT
 * Independently authored, bounded UTF-16LE text conversion for decoded CAD
 * strings. This is not an AC1032 string or entity record parser.
 */
#ifndef MUSA_DWG1032_TEXT_UTF16LE_H
#define MUSA_DWG1032_TEXT_UTF16LE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Convert an explicitly bounded, non-NUL-terminated UTF-16LE payload to
 * NUL-terminated UTF-8. byte_count must be even; surrogate pairs must be
 * well-formed. Embedded U+0000 is rejected to prevent truncated labels.
 * Caller removes any on-disk terminator before invoking this function.
 * On success *written is the UTF-8 byte count excluding the terminator.
 * On failure output and *written remain unchanged. The full destination
 * capacity must not overlap input or written. No allocation; O(n) time,
 * O(1) auxiliary memory. Does not parse or authenticate DWG text records. */
/* Preflight a caller-authenticated UTF-16LE field before allocating text
 * storage. On success *required includes the trailing UTF-8 NUL byte;
 * empty input requires one byte. Rejects invalid Unicode, embedded NUL,
 * odd sizes, aliasing and arithmetic overflow without changing *required.
 * Does not infer a DWG text schema or own any memory. */
int musa_dwg1032_text_utf16le_measure(
    const uint8_t *utf16le, size_t byte_count, size_t *required);

int musa_dwg1032_text_utf16le_to_utf8(
    const uint8_t *utf16le, size_t byte_count,
    char *utf8, size_t capacity, size_t *written);

/* Consume exactly byte_count bytes of a caller-verified UTF-16LE field
 * from a byte-aligned, bounded record cursor and decode to UTF-8.
 * This bridges already-decoded record payloads to display text; it does NOT
 * discover AC1032 text lengths, encoding flags or entity types.
 * A failed conversion (including malformed surrogate pairs, odd byte count,
 * truncated input or inadequate output capacity) leaves cursor, output and
 * *written untouched. The entire source cursor buffer, cursor metadata and
 * output storage must be disjoint. Zero-length text produces an empty string.
 * No allocation, O(byte_count), and no intermediate UTF-16 copy. */
#include "musa_dwg1032_cursor.h"
int musa_dwg1032_text_utf16le_take(
    MusaDwg1032Cursor *record, size_t byte_count,
    char *utf8, size_t capacity, size_t *written);

/* Read-only sizing query for a byte-aligned, caller-authenticated record
 * field. Does not advance the cursor or publish required bytes on failure.
 * The entire source cursor storage and metadata must be disjoint from
 * required. No heap, no schema inference, O(byte_count) time. */
int musa_dwg1032_text_utf16le_peek_measure(
    const MusaDwg1032Cursor *record, size_t byte_count, size_t *required);

#ifdef __cplusplus
}
#endif
#endif
