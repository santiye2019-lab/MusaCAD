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
#ifdef __cplusplus
}
#endif
#endif
