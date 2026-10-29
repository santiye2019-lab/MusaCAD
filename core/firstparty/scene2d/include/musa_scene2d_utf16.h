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
#ifdef __cplusplus
}
#endif
#endif
