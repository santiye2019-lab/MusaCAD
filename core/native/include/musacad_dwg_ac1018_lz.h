/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_LZ_H
#define MUSACAD_DWG_AC1018_LZ_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded, read-only first-party R2004 page LZ stream decoder.
 * A source page may include padding after the 0x11 terminal opcode:
 * consumed_bytes reports precisely the bytes used, not padded length.
 * Output and consumed_bytes remain unchanged on any invalid/truncated
 * stream. Decoder is NOT proof of DWG section checksum/locator integrity.
 * Maximum expanded stream length: 1 MiB. */
int musa_dwg_ac1018_lz_decode(const uint8_t *source, size_t compressed_bytes,
                              uint8_t *output, size_t decompressed_bytes,
                              size_t *consumed_bytes);
#ifdef __cplusplus
}
#endif
#endif
