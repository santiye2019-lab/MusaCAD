/* SPDX-License-Identifier: MIT
 * Independently authored UTF-16LE to UTF-8 adapter for predecoded CAD text.
 */
#ifndef MUSA_SCENE2D_UTF16_H
#define MUSA_SCENE2D_UTF16_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Converts a bounded, un-terminated UTF-16LE payload to strict UTF-8.
 * Rejects odd lengths, embedded NULs and unpaired surrogates. No DWG
 * sections, text formatting, glyphs or code-page conversion are decoded.
 * With utf8=NULL and capacity=0, validates and reports required bytes.
 * Otherwise capacity must include one byte for the final NUL terminator.
 * *written excludes that terminator. No allocation; failure preserves both
 * output bytes and *written. Input/output/count storage must not overlap. */
int musa_scene2d_utf16le_to_utf8(const uint8_t *utf16le,
                                 size_t byte_length,
                                 uint8_t *utf8,
                                 size_t utf8_capacity,
                                 size_t *written);
/* Converts a NUL-terminated UTF-16LE field within a bounded byte record.
 * The first aligned 00 00 code unit terminates the field; bytes after it
 * belong to the enclosing record and are not decoded. The bound may be odd
 * when the next field starts immediately after a complete string.
 * On success, *consumed includes the two-byte terminator; *written is the
 * UTF-8 byte count excluding its terminator. Size-query mode is supported.
 * No terminator, invalid Unicode, aliasing or insufficient capacity fails
 * without modifying output, *written or *consumed. No DWG record framing
 * or version-specific string layout is inferred by this adapter. */
int musa_scene2d_utf16le_cstr_to_utf8(const uint8_t *record,
                                      size_t record_bytes,
                                      uint8_t *utf8,
                                      size_t utf8_capacity,
                                      size_t *written,
                                      size_t *consumed);
/* Convert an externally length-verified UTF-16LE field inside a bounded
 * record. code_units counts UTF-16 code units (not Unicode scalars), and
 * *consumed reports exactly code_units * 2 bytes; trailing record bytes
 * remain untouched. No length prefix or DWG string layout is inferred.
 * Empty fields are valid. Size-query mode uses utf8=NULL/capacity=0.
 * On failure output, *written and *consumed remain unchanged. All input
 * record bytes, output and count storage must be nonoverlapping. */
int musa_scene2d_utf16le_counted_to_utf8(const uint8_t *record,
                                         size_t record_bytes,
                                         size_t code_units,
                                         uint8_t *utf8,
                                         size_t utf8_capacity,
                                         size_t *written,
                                         size_t *consumed);
/* Decode a caller-verified counted UTF-16LE field starting at byte_offset
 * inside the complete record. The byte offset may be odd (for example
 * after a packed prefix); the field itself is always UTF-16LE.
 * On success, *consumed is the absolute byte offset immediately AFTER the
 * field, not the field length. No DWG framing or length-prefix interpretation.
 * The entire record must be disjoint from output and count storage, even
 * bytes before/after the selected field. Size queries and empty fields work.
 * Failure preserves output, *written and *consumed. */
int musa_scene2d_utf16le_counted_at_to_utf8(const uint8_t *record,
                                            size_t record_bytes,
                                            size_t byte_offset,
                                            size_t code_units,
                                            uint8_t *utf8,
                                            size_t utf8_capacity,
                                            size_t *written,
                                            size_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
