/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_BITS_H
#define MUSACAD_BITS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * First-party, platform-neutral bounded binary bit stream.
 *
 * Bits of each value are encoded least-significant-bit first within the
 * current byte, in ascending bit offset order. This is an INTERNAL primitive,
 * not an assertion about any DWG release's bit-code layout. Actual DWG page,
 * checksum, section and entity decoding are NOT implemented by this module.
 *
 * No allocations, no out-of-bounds access and no partial cursor advancement
 * on invalid/truncated reads or writes. Reader/writer permissions are strict.
 */
typedef struct MusaBits {
    const uint8_t *read_bytes;
    uint8_t *write_bytes;
    size_t byte_count, bit_position;
} MusaBits;
int musa_bits_reader(MusaBits *cursor,const uint8_t *src,size_t bytes);
int musa_bits_writer(MusaBits *cursor,uint8_t *dst,size_t bytes);
size_t musa_bits_remaining(const MusaBits *cursor);
int musa_bits_seek(MusaBits *cursor,size_t bit_position);
int musa_bits_align_byte(MusaBits *cursor);
int musa_bits_read(MusaBits *cursor,unsigned width,uint64_t *value);
int musa_bits_write(MusaBits *cursor,unsigned width,uint64_t value);
/* Canonical unsigned LEB128 utility. It is NOT a DWG proprietary bitcode. */
int musa_bits_read_uleb128(MusaBits *cursor,uint64_t *value);
int musa_bits_write_uleb128(MusaBits *cursor,uint64_t value);
#ifdef __cplusplus
}
#endif
#endif
