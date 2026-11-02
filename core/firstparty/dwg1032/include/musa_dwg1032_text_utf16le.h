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
int musa_dwg1032_text_utf16le_to_utf8(
    const uint8_t *utf16le, size_t byte_count,
    char *utf8, size_t capacity, size_t *written);

#ifdef __cplusplus
}
#endif
#endif
